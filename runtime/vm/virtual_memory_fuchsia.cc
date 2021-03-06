// Copyright (c) 2016, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

#include "vm/globals.h"
#if defined(TARGET_OS_FUCHSIA)

#include "vm/virtual_memory.h"

#include <magenta/syscalls.h>
#include <unistd.h>  // NOLINT

#include "platform/assert.h"
#include "vm/memory_region.h"
#include "vm/os.h"

namespace dart {

uword VirtualMemory::page_size_ = 0;


void VirtualMemory::InitOnce() {
  page_size_ = getpagesize();
}


VirtualMemory* VirtualMemory::ReserveInternal(intptr_t size) {
  mx_handle_t vmo = _magenta_vm_object_create(size);
  if (vmo <= 0) {
    return NULL;
  }

  // TODO(zra): map with PERM_NONE, when that works, and relax with
  // Commit and Protect when they are implemented.
  // Issue MG-161.
  const int prot = MX_VM_FLAG_PERM_READ |
                   MX_VM_FLAG_PERM_WRITE |
                   MX_VM_FLAG_PERM_EXECUTE;
  uintptr_t addr;
  mx_status_t status = _magenta_process_vm_map(0, vmo, 0, size, &addr, prot);
  if (status != NO_ERROR) {
    _magenta_handle_close(vmo);
    FATAL("VirtualMemory::ReserveInternal FAILED");
    return NULL;
  }

  MemoryRegion region(reinterpret_cast<void*>(addr), size);
  return new VirtualMemory(region, vmo);
}


VirtualMemory::~VirtualMemory() {
  if (!embedder_allocated()) {
    // TODO(zra): Use reserved_size_.
    // Issue MG-162.
    mx_status_t status = _magenta_process_vm_unmap(
        0, reinterpret_cast<uintptr_t>(address()), 0 /*reserved_size_*/);
    if (status != NO_ERROR) {
      FATAL("VirtualMemory::~VirtualMemory: unamp FAILED");
    }

    status = _magenta_handle_close(handle());
    if (status != NO_ERROR) {
      FATAL("VirtualMemory::~VirtualMemory: handle_close FAILED");
    }
  }
}


bool VirtualMemory::FreeSubSegment(void* address, intptr_t size) {
  UNIMPLEMENTED();
  return false;
}


bool VirtualMemory::Commit(uword addr, intptr_t size, bool executable) {
  // TODO(zra): Implement when the protections for a mapping can be changed.
  // Issue MG-133.
  return true;
}


bool VirtualMemory::Protect(void* address, intptr_t size, Protection mode) {
  // TODO(zra): Implement when Fuchsia has an mprotect-like call.
  return true;
}

}  // namespace dart

#endif  // defined(TARGET_OS_FUCHSIA)
