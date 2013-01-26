#ifndef SOFTDSP_DATA_LAYOUT_GET_ELEMENT_TYPE_HPP
#define SOFTDSP_DATA_LAYOUT_GET_ELEMENT_TYPE_HPP

#include <softdsp/primitive.hpp>
#include <softdsp/data_layout/proxy.hpp>

namespace softdsp {
  namespace data_layout {

    template< typename T, bool is_primitive = is_primitive< T >::value >
    struct get_element_type {};

    template< typename T >
    struct get_element_type< T, true > {
      typedef proxy< T > type;
    };

    template< typename T >
    struct get_element_type< T, false > {
      typedef T type;
    };
  }
}

#endif
