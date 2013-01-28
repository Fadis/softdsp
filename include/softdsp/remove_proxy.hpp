#ifndef SOFTDSP_REMOVE_PROXY_HPP
#define SOFTDSP_REMOVE_PROXY_HPP

#include <boost/mpl/bool.hpp>

namespace softdsp {
  namespace data_layout {
    template< typename T >
    class proxy;
  }
  namespace detail {
    template< typename T >
    boost::mpl::bool_< true > is_proxy( data_layout::proxy< T > );
    boost::mpl::bool_< false > is_proxy( ... );
  }
  template< typename T >
  struct is_proxy
  : public decltype( detail::is_proxy( std::declval< T >() ) ) {};

  template< typename T, bool is_proxy_ = is_proxy< T >::value >
  struct remove_proxy {};
  template< typename T >
  struct remove_proxy< data_layout::proxy< T >, true > {
    typedef T type;
  };
  template< typename T >
  struct remove_proxy< T, false > {
    typedef T type;
  };
}

#endif
