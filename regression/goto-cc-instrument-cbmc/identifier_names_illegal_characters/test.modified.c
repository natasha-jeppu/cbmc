// __atomic_exchange_l_ptr__start_sub_lV_end_sub_$lV$
// 
void __atomic_exchange_l_ptr__start_sub_lV_end_sub_$lV$(void **p_0, void **p_1, void **p_2, signed int p_3);
// __atomic_exchange_n_l_ptr__start_sub_lV_end_sub_$lV$
// file test.c line 6 function main
void * __atomic_exchange_n_l_ptr__start_sub_lV_end_sub_$lV$(void **p_0, void *p_1, signed int p_2);
// __atomic_store_l_ptr__start_sub_lV_end_sub_$lV$
// 
void __atomic_store_l_ptr__start_sub_lV_end_sub_$lV$(void **p_0, void **p_1, signed int p_2);
// __atomic_store_n_l_ptr__start_sub_lV_end_sub_$lV$
// file test.c line 5 function main
void __atomic_store_n_l_ptr__start_sub_lV_end_sub_$lV$(void **p_0, void *p_1, signed int p_2);
// __atomic_thread_fence
// file <builtin-architecture-strings> line 20
void __atomic_thread_fence(signed int);

// __atomic_exchange_l_ptr__start_sub_lV_end_sub_$lV$
// 
void __atomic_exchange_l_ptr__start_sub_lV_end_sub_$lV$(void **p_0, void **p_1, void **p_2, signed int p_3)
{
  __CPROVER_atomic_begin();
  *p_2 = *p_0;
  *p_0 = *p_1;
  __atomic_thread_fence(p_3);
  __CPROVER_atomic_end();
}

// __atomic_exchange_n_l_ptr__start_sub_lV_end_sub_$lV$
// file test.c line 6 function main
void * __atomic_exchange_n_l_ptr__start_sub_lV_end_sub_$lV$(void **p_0, void *p_1, signed int p_2)
{
  void *result;
  __atomic_exchange_l_ptr__start_sub_lV_end_sub_$lV$(p_0, &p_1, &result, p_2);
  return result;
}

// __atomic_store_l_ptr__start_sub_lV_end_sub_$lV$
// 
void __atomic_store_l_ptr__start_sub_lV_end_sub_$lV$(void **p_0, void **p_1, signed int p_2)
{
  __CPROVER_atomic_begin();
  *p_0 = *p_1;
  __atomic_thread_fence(p_2);
  __CPROVER_atomic_end();
}

// __atomic_store_n_l_ptr__start_sub_lV_end_sub_$lV$
// file test.c line 5 function main
void __atomic_store_n_l_ptr__start_sub_lV_end_sub_$lV$(void **p_0, void *p_1, signed int p_2)
{
  __atomic_store_l_ptr__start_sub_lV_end_sub_$lV$(p_0, &p_1, p_2);
}

// main
// file test.c line 1
signed int main(void)
{
  void *v;
  void *x;
  void *x_before=x;
  __atomic_store_n_l_ptr__start_sub_lV_end_sub_$lV$(&x, (void *)42, 0);
  __atomic_exchange_n_l_ptr__start_sub_lV_end_sub_$lV$(&x, (void *)42, 0);
}

