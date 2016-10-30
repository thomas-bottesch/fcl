from libc.stdint cimport UINT32_MAX

cdef cdict_to_python_dict(cdict* _d):
  d = {}
  while _d != NULL:
    key_name = _d.name.decode() if (type(_d.name) != str) else _d.name
    
    if _d.type == DICT_I:
      if (_d.val <= UINT32_MAX):
        d[key_name] = int(_d.val)
      else:
        d[key_name] = _d.val
    elif _d.type == DICT_F:
      d[key_name] = _d.fval
    elif _d.type == DICT_ST:
      if type(_d.sval) != str:
          d[key_name] = _d.sval.decode()
      else:
          d[key_name] = _d.sval
    elif _d.type == DICT_D:
      d[key_name] = cdict_to_python_dict(_d.d_val)
    elif _d.type == DICT_FL:
      d[key_name] = []
      for i in range(_d.val):
        d[key_name].append(_d.fval_list[i])
    elif _d.type == DICT_IL:
      d[key_name] = []
      all_uint32 = True
      for i in range(_d.val):
        if (_d.val_list[i] > UINT32_MAX):
          all_uint32 = False
          
      for i in range(_d.val):
        if (all_uint32):
          d[key_name].append(int(_d.val_list[i]))
        else:
          d[key_name].append(_d.val_list[i])
        
    elif _d.type == DICT_SL:
      d[key_name] = []
      for i in range(_d.val):
        if type(_d.sval_list[i]) != str:
            d[key_name].append(_d.sval_list[i].decode())
        else:
            d[key_name].append(_d.sval_list[i])
    elif _d.type == DICT_DL:
      d[key_name] = []
      for i in range(_d.val):
        d[key_name].append(cdict_to_python_dict(_d.d_list[i]))
              
    _d = d_next(_d)
  return d
