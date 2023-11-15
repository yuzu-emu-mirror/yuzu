# SPDX-FileCopyrightText: 2018 yuzu Emulator Project
# SPDX-License-Identifier: GPL-2.0-or-later

add_library(kernel STATIC
    board/nintendo/nx/k_memory_layout.cpp
    board/nintendo/nx/k_memory_layout.h
    board/nintendo/nx/k_system_control.cpp
    board/nintendo/nx/k_system_control.h
    board/nintendo/nx/secure_monitor.h
    code_set.cpp
    code_set.h
    svc_results.h
    global_scheduler_context.cpp
    global_scheduler_context.h
    init/init_slab_setup.cpp
    init/init_slab_setup.h
    initial_process.h
    k_address_arbiter.cpp
    k_address_arbiter.h
    k_address_space_info.cpp
    k_address_space_info.h
    k_auto_object.cpp
    k_auto_object.h
    k_auto_object_container.cpp
    k_auto_object_container.h
    k_affinity_mask.h
    k_capabilities.cpp
    k_capabilities.h
    k_class_token.cpp
    k_class_token.h
    k_client_port.cpp
    k_client_port.h
    k_client_session.cpp
    k_client_session.h
    k_code_memory.cpp
    k_code_memory.h
    k_condition_variable.cpp
    k_condition_variable.h
    k_debug.h
    k_device_address_space.cpp
    k_device_address_space.h
    k_dynamic_page_manager.h
    k_dynamic_resource_manager.h
    k_dynamic_slab_heap.h
    k_event.cpp
    k_event.h
    k_event_info.h
    k_handle_table.cpp
    k_handle_table.h
    k_hardware_timer_base.h
    k_hardware_timer.cpp
    k_hardware_timer.h
    k_interrupt_manager.cpp
    k_interrupt_manager.h
    k_light_condition_variable.cpp
    k_light_condition_variable.h
    k_light_lock.cpp
    k_light_lock.h
    k_memory_block.h
    k_memory_block_manager.cpp
    k_memory_block_manager.h
    k_memory_layout.cpp
    k_memory_layout.h
    k_memory_manager.cpp
    k_memory_manager.h
    k_memory_region.h
    k_memory_region_type.h
    k_object_name.cpp
    k_object_name.h
    k_page_bitmap.h
    k_page_buffer.cpp
    k_page_buffer.h
    k_page_heap.cpp
    k_page_heap.h
    k_page_group.cpp
    k_page_group.h
    k_page_table.h
    k_page_table_base.cpp
    k_page_table_base.h
    k_page_table_manager.h
    k_page_table_slab_heap.h
    k_port.cpp
    k_port.h
    k_priority_queue.h
    k_process.cpp
    k_process.h
    k_process_page_table.h
    k_readable_event.cpp
    k_readable_event.h
    k_resource_limit.cpp
    k_resource_limit.h
    k_scheduler.cpp
    k_scheduler.h
    k_scheduler_lock.h
    k_scoped_lock.h
    k_scoped_resource_reservation.h
    k_scoped_scheduler_lock_and_sleep.h
    k_server_port.cpp
    k_server_port.h
    k_server_session.cpp
    k_server_session.h
    k_session.cpp
    k_session.h
    k_session_request.cpp
    k_session_request.h
    k_shared_memory.cpp
    k_shared_memory.h
    k_shared_memory_info.h
    k_slab_heap.h
    k_spin_lock.cpp
    k_spin_lock.h
    k_synchronization_object.cpp
    k_synchronization_object.h
    k_system_control.h
    k_system_resource.cpp
    k_system_resource.h
    k_thread.cpp
    k_thread.h
    k_thread_local_page.cpp
    k_thread_local_page.h
    k_thread_queue.cpp
    k_thread_queue.h
    k_timer_task.h
    k_trace.h
    k_transfer_memory.cpp
    k_transfer_memory.h
    k_typed_address.h
    k_worker_task.h
    k_worker_task_manager.cpp
    k_worker_task_manager.h
    kernel.cpp
    kernel.h
    memory_types.h
    message_buffer.h
    physical_core.cpp
    physical_core.h
    physical_memory.h
    slab_helpers.h
    svc.cpp
    svc.h
    svc_common.h
    svc_types.h
    svc_version.h
    svc/svc_activity.cpp
    svc/svc_address_arbiter.cpp
    svc/svc_address_translation.cpp
    svc/svc_cache.cpp
    svc/svc_code_memory.cpp
    svc/svc_condition_variable.cpp
    svc/svc_debug.cpp
    svc/svc_debug_string.cpp
    svc/svc_device_address_space.cpp
    svc/svc_event.cpp
    svc/svc_exception.cpp
    svc/svc_info.cpp
    svc/svc_insecure_memory.cpp
    svc/svc_interrupt_event.cpp
    svc/svc_io_pool.cpp
    svc/svc_ipc.cpp
    svc/svc_kernel_debug.cpp
    svc/svc_light_ipc.cpp
    svc/svc_lock.cpp
    svc/svc_memory.cpp
    svc/svc_physical_memory.cpp
    svc/svc_port.cpp
    svc/svc_power_management.cpp
    svc/svc_process.cpp
    svc/svc_process_memory.cpp
    svc/svc_processor.cpp
    svc/svc_query_memory.cpp
    svc/svc_register.cpp
    svc/svc_resource_limit.cpp
    svc/svc_secure_monitor_call.cpp
    svc/svc_session.cpp
    svc/svc_shared_memory.cpp
    svc/svc_synchronization.cpp
    svc/svc_thread.cpp
    svc/svc_thread_profiler.cpp
    svc/svc_tick.cpp
    svc/svc_transfer_memory.cpp
)

if (MSVC)
    target_compile_options(kernel PRIVATE
        /we4242 # 'identifier': conversion from 'type1' to 'type2', possible loss of data
        /we4244 # 'conversion': conversion from 'type1' to 'type2', possible loss of data
        /we4245 # 'conversion': conversion from 'type1' to 'type2', signed/unsigned mismatch
        /we4254 # 'operator': conversion from 'type1:field_bits' to 'type2:field_bits', possible loss of data
        /we4800 # Implicit conversion from 'type' to bool. Possible information loss
    )
else()
    target_compile_options(kernel PRIVATE
        -Werror=conversion

        -Wno-sign-conversion
        -Wno-cast-function-type

        $<$<CXX_COMPILER_ID:Clang>:-fsized-deallocation>
    )
endif()

create_target_directory_groups(kernel)

target_link_libraries(kernel PUBLIC common)
target_link_libraries(kernel PUBLIC Boost::headers PRIVATE fmt::fmt)

if (ARCHITECTURE_x86_64 OR ARCHITECTURE_arm64)
    target_link_libraries(kernel PRIVATE core dynarmic::dynarmic)
endif()

if (YUZU_USE_PRECOMPILED_HEADERS)
    target_precompile_headers(kernel PRIVATE precompiled_headers.h)
endif()

if (YUZU_ENABLE_LTO)
  set_property(TARGET kernel PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()
