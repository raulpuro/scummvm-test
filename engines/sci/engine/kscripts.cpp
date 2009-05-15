/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * $URL$
 * $Id$
 *
 */

#include "sci/sci.h"
#include "sci/scicore/resource.h"
#include "sci/engine/state.h"
#include "sci/engine/kernel_types.h"
#include "sci/engine/kernel.h"

namespace Sci {

reg_t read_selector(EngineState *s, reg_t object, Selector selector_id, const char *file, int line) {
	reg_t *address;

	if (lookup_selector(s, object, selector_id, &address, NULL) != kSelectorVariable)
		return NULL_REG;
	else
		return *address;
}

void write_selector(EngineState *s, reg_t object, Selector selector_id, reg_t value, const char *fname, int line) {
	reg_t *address;

	if ((selector_id < 0) || (selector_id > (int)s->_selectorNames.size())) {
		warning("Attempt to write to invalid selector %d of"
		         " object at "PREG" (%s L%d).", selector_id, PRINT_REG(object), fname, line);
		return;
	}

	if (lookup_selector(s, object, selector_id, &address, NULL) != kSelectorVariable)
		warning("Selector '%s' of object at "PREG" could not be"
		         " written to (%s L%d)", s->_selectorNames[selector_id].c_str(), PRINT_REG(object), fname, line);
	else
		*address = value;
}

int invoke_selector(EngineState *s, reg_t object, int selector_id, int noinvalid, int kfunct,
	StackPtr k_argp, int k_argc, const char *fname, int line, int argc, ...) {
	va_list argp;
	int i;
	int framesize = 2 + 1 * argc;
	reg_t address;
	int slc_type;
	StackPtr stackframe = k_argp + k_argc;

	// Execution stack
	ExecStack *xstack;

	stackframe[0] = make_reg(0, selector_id);  // The selector we want to call
	stackframe[1] = make_reg(0, argc); // Argument count

	slc_type = lookup_selector(s, object, selector_id, NULL, &address);

	if (slc_type == kSelectorNone) {
		SCIkwarn(SCIkERROR, "Selector '%s' of object at "PREG" could not be invoked (%s L%d)\n",
		         s->_selectorNames[selector_id].c_str(), PRINT_REG(object), fname, line);
		if (noinvalid == 0)
			KERNEL_OOPS("Not recoverable: VM was halted\n");
		return 1;
	}
	if (slc_type == kSelectorVariable) // Swallow silently
		return 0;

	va_start(argp, argc);
	for (i = 0; i < argc; i++) {
		reg_t arg = va_arg(argp, reg_t);
		stackframe[2 + i] = arg; // Write each argument
	}
	va_end(argp);

	// Write "kernel" call to the stack, for debugging:
	xstack = add_exec_stack_entry(s, NULL_REG, NULL, NULL_REG, k_argc, k_argp - 1, 0, NULL_REG,
	                              s->execution_stack_pos, SCI_XS_CALLEE_LOCALS);
	xstack->selector = -42 - kfunct; // Evil debugging hack to identify kernel function
	xstack->type = EXEC_STACK_TYPE_KERNEL;

	// Now commit the actual function:
	xstack = send_selector(s, object, object, stackframe, framesize, stackframe);

	xstack->sp += argc + 2;
	xstack->fp += argc + 2;

	run_vm(s, 0); // Start a new vm

	--(s->execution_stack_pos); // Get rid of the extra stack entry

	return 0;
}

bool is_object(EngineState *s, reg_t object) {
	return obj_get(s, object) != NULL;
}

// Loads arbitrary resources of type 'restype' with resource numbers 'resnrs'
// This implementation ignores all resource numbers except the first one.
reg_t kLoad(EngineState *s, int funct_nr, int argc, reg_t *argv) {
	int restype = KP_UINT(argv[0]);
	int resnr = KP_UINT(argv[1]);

	// Request to dynamically allocate hunk memory for later use
	if (restype == kResourceTypeMemory)
		return kalloc(s, "kLoad()", resnr);

	return make_reg(0, ((restype << 11) | resnr)); // Return the resource identifier as handle
}

reg_t kLock(EngineState *s, int funct_nr, int argc, reg_t *argv) {
	int restype = UKPV(0) & 0x7f;
	int resnr = UKPV(1);
	int state = argc > 2 ? UKPV(2) : 1;

	Resource *which;

	switch (state) {
	case 1 :
		s->resmgr->findResource((ResourceType)restype, resnr, 1);
		break;
	case 0 :
		which = s->resmgr->findResource((ResourceType)restype, resnr, 0);
		s->resmgr->unlockResource(which, resnr, (ResourceType)restype);
		break;
	}
	return s->r_acc;
}

// Unloads an arbitrary resource of type 'restype' with resource numbber 'resnr'
reg_t kUnLoad(EngineState *s, int funct_nr, int argc, reg_t *argv) {
	int restype = KP_UINT(argv[0]);
	reg_t resnr = argv[1];

	if (restype == kResourceTypeMemory)
		kfree(s, resnr);

	return s->r_acc;
}

reg_t kClone(EngineState *s, int funct_nr, int argc, reg_t *argv) {
	reg_t parent_addr = argv[0];
	Object *parent_obj = obj_get(s, parent_addr);
	reg_t clone_addr;
	Clone *clone_obj; // same as Object*

	if (!parent_obj) {
		SCIkwarn(SCIkERROR, "Attempt to clone non-object/class at "PREG" failed", PRINT_REG(parent_addr));
		return NULL_REG;
	}

	SCIkdebug(SCIkMEM, "Attempting to clone from "PREG"\n", PRINT_REG(parent_addr));

	clone_obj = s->seg_manager->alloc_Clone(&clone_addr);

	if (!clone_obj) {
		SCIkwarn(SCIkERROR, "Cloning "PREG" failed-- internal error!\n", PRINT_REG(parent_addr));
		return NULL_REG;
	}

	*clone_obj = *parent_obj;
	clone_obj->flags = 0;

	// Mark as clone
	clone_obj->_variables[SCRIPT_INFO_SELECTOR].offset = SCRIPT_INFO_CLONE;
	clone_obj->_variables[SCRIPT_SPECIES_SELECTOR] = clone_obj->pos;
	if (IS_CLASS(parent_obj))
		clone_obj->_variables[SCRIPT_SUPERCLASS_SELECTOR] = parent_obj->pos;
	s->seg_manager->getScript(parent_obj->pos.segment, SEG_ID)->incrementLockers();
	s->seg_manager->getScript(clone_obj->pos.segment, SEG_ID)->incrementLockers();

	return clone_addr;
}

extern void _k_view_list_mark_free(EngineState *s, reg_t off);

reg_t kDisposeClone(EngineState *s, int funct_nr, int argc, reg_t *argv) {
	reg_t victim_addr = argv[0];
	Clone *victim_obj = obj_get(s, victim_addr);
	uint16 underBits;

	if (!victim_obj) {
		SCIkwarn(SCIkERROR, "Attempt to dispose non-class/object at "PREG"\n",
		         PRINT_REG(victim_addr));
		return s->r_acc;
	}

	if (victim_obj->_variables[SCRIPT_INFO_SELECTOR].offset != SCRIPT_INFO_CLONE) {
		//SCIkwarn("Attempt to dispose something other than a clone at %04x\n", offset);
		// SCI silently ignores this behaviour; some games actually depend on it
		return s->r_acc;
	}

	underBits = GET_SEL32V(victim_addr, underBits);
	if (underBits) {
		warning("Clone "PREG" was cleared with underBits set", PRINT_REG(victim_addr));
	}
#if 0
	if (s->dyn_views) {  // Free any widget associated with the clone
		GfxWidget *widget = gfxw_set_id(gfxw_remove_ID(s->dyn_views, offset), GFXW_NO_ID);

		if (widget && s->bg_widgets)
			s->bg_widgets->add(GFXWC(s->bg_widgets), widget);
	}
#endif

	victim_obj->flags |= OBJECT_FLAG_FREED;

	_k_view_list_mark_free(s, victim_addr); // Free on view list, if neccessary

	return s->r_acc;
}

// Returns script dispatch address index in the supplied script
reg_t kScriptID(EngineState *s, int funct_nr, int argc, reg_t *argv) {
	int script = KP_UINT(argv[0]);
	int index = KP_UINT(KP_ALT(1, NULL_REG));

	SegmentId scriptid = script_get_segment(s, script, SCRIPT_GET_LOAD);
	Script *scr;

	if (argv[0].segment)
		return argv[0];

	if (!scriptid)
		return NULL_REG;

	scr = s->seg_manager->getScript(scriptid, SEG_ID);

	if (!scr->exports_nr) {
		SCIkdebug(SCIkERROR, "Script 0x%x does not have a dispatch table\n", script);
		return NULL_REG;
	}

	if (index > scr->exports_nr) {
		SCIkwarn(SCIkERROR, "Dispatch index too big: %d > %d\n", index, scr->exports_nr);
		return NULL_REG;
	}

	return make_reg(scriptid, s->seg_manager->validateExportFunc(index, scriptid));
}

reg_t kDisposeScript(EngineState *s, int funct_nr, int argc, reg_t *argv) {
	int script = argv[0].offset;

	// Work around QfG1 graveyard bug
	if (argv[0].segment)
		return s->r_acc;

	if (s->seg_manager->scriptIsLoaded(script, SCRIPT_ID)) {
		int id = s->seg_manager->segGet(script);

		if (s->_executionStack[s->execution_stack_pos].addr.pc.segment != id)
			s->seg_manager->getScript(id, SEG_ID)->setLockers(1);
	}

	script_uninstantiate(s, script);
	s->_executionStackPosChanged = true;
	return s->r_acc;
}

int is_heap_object(EngineState *s, reg_t pos) {
	Object *obj = obj_get(s, pos);
	return (obj != NULL && (!(obj->flags & OBJECT_FLAG_FREED)) && (!s->seg_manager->scriptIsMarkedAsDeleted(pos.segment)));
}

reg_t kIsObject(EngineState *s, int funct_nr, int argc, reg_t *argv) {
	if (argv[0].offset == 0xffff) // Treated specially
		return NULL_REG;
	else
		return make_reg(0, is_heap_object(s, argv[0]));
}

reg_t kRespondsTo(EngineState *s, int funct_nr, int argc, reg_t *argv) {
	reg_t obj = argv[0];
	int selector = KP_UINT(argv[1]);

	return make_reg(0, is_heap_object(s, obj) && lookup_selector(s, obj, selector, NULL, NULL) != kSelectorNone);
}

} // End of namespace Sci
