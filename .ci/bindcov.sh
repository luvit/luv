#!/usr/bin/env bash

unbound_functions=0
skipped=()

# false positives
skipped+=(uv_thread_create uv_thread_create_ex)

# intentionally not bound
skipped+=(uv_replace_allocator)

# threading/synchronization
skipped+=(
	uv_mutex_init uv_mutex_init_recursive uv_mutex_destroy uv_mutex_lock uv_mutex_trylock
	uv_mutex_unlock uv_rwlock_init uv_rwlock_destroy uv_rwlock_rdlock uv_rwlock_tryrdlock 
	uv_rwlock_rdunlock uv_rwlock_wrlock uv_rwlock_trywrlock uv_rwlock_wrunlock uv_sem_init
	uv_sem_destroy uv_sem_post uv_sem_wait uv_sem_trywait uv_cond_init uv_cond_destroy
	uv_cond_signal uv_cond_broadcast uv_barrier_init uv_barrier_destroy uv_barrier_wait
	uv_cond_wait uv_cond_timedwait uv_once uv_key_create uv_key_delete uv_key_get uv_key_set
)

# yet to be implemented / ruled out
# https://github.com/luvit/luv/issues/410
skipped+=(
	uv_loop_configure uv_setup_args uv_default_loop uv_loop_new uv_loop_delete
	uv_loop_size uv_loop_fork uv_loop_get_data uv_loop_set_data uv_strerror uv_strerror_r uv_err_name
	uv_err_name_r uv_handle_size uv_handle_get_type uv_handle_type_name uv_handle_get_data
	uv_handle_get_loop uv_handle_set_data uv_req_size uv_req_get_data uv_req_set_data uv_req_get_type
	uv_req_type_name uv_udp_set_source_membership uv_pipe_chmod uv_process_get_pid uv_get_osfhandle
	uv_open_osfhandle uv_fs_get_type uv_fs_get_result uv_fs_get_ptr uv_fs_get_path uv_fs_get_statbuf
	uv_ip4_addr uv_ip6_addr uv_ip4_name uv_ip6_name uv_inet_ntop uv_inet_pton uv_dlopen uv_dlclose
	uv_dlsym uv_dlerror
)

# get all public uv_ functions from uv.h
for fn in `grep -oP "UV_EXTERN [^\(]+ uv_[^\(]+\(" deps/libuv/include/uv.h | sed 's/($//' | grep -oP "[^ ]+$"`; do
	# skip everything in the skipped array and any initialization/cleanup fns
	if [[ " ${skipped[@]} " =~ " ${fn} " || $fn == *_init* || $fn == *_free* || $fn == *_cleanup ]] ; then
		continue
	fi
	# count all uses
	count=`grep -o "$fn" src/*.c | wc -l`
	# check if a luv_ version exists
	grep -Fq "l$fn" src/*.c
	bound=$?
	# not bound
	if [ ! $bound -eq 0 ] ; then
		# not used
		if [ $count -eq 0 ] ; then
			echo $fn
		else
			echo "$fn (used internally but not bound externally)"
		fi
		unbound_functions=$((unbound_functions+1))
	fi
done

exit $unbound_functions
