#ifndef SOFTDSP_NTH_TYPE_HPP
#define SOFTDSP_NTH_TYPE_HPP

namespace softdsp {
  template < size_t index, typename head, typename... tail >
  struct nth_type {
    typedef typename nth_type< index - 1, tail... >::type type;
  };
  template < typename head, typename... tail >
  struct nth_type< 0, head, tail... > {
    typedef head type;
  };
}

#endif

