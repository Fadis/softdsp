#ifndef SOFTDSP_RETURN_VALUE_HPP
#define SOFTDSP_RETURN_VALUE_HPP

#include <llvm/Value.h>

#include <softdsp/primitive.hpp>
#include <softdsp/remove_proxy.hpp>

namespace softdsp {
  struct return_value_ {};

  template< typename T >
  struct return_value : public return_value_ {
    return_value( llvm::Value *value_ ) : value( value_ ) {}
    llvm::Value * const value;
  };

  template< typename T >
  struct get_return_type {
    typedef T type;
  };
  template< typename T >
  struct get_return_type< return_value< T > > {
    typedef typename remove_proxy< T >::type type;
  };
  template< typename T >
  struct get_return_type< return_value< T >& > {
    typedef T type;
  };
  
  template< typename T >
  struct is_primitive< return_value< T > >
  : public boost::mpl::or_<
    boost::is_integral<
      typename boost::remove_cv<
        T
      >::type
    >,
    boost::is_floating_point<
      typename boost::remove_cv<
        T
      >::type
    >
  >{};
}

#endif
