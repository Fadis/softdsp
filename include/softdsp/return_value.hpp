#ifndef SOFTDSP_RETURN_VALUE_HPP
#define SOFTDSP_RETURN_VALUE_HPP

#include <llvm/Value.h>

namespace softdsp {
  struct return_value_ {};

  template< typename T >
  struct return_value : public return_value_ {
    return_value( llvm::Value *value_ ) : value( value_ ) {}
    llvm::Value *value;
  };

  template< typename T >
  struct get_return_type {
    typedef T type;
  };
  template< typename T >
  struct get_return_type< return_value< T > > {
    typedef T type;
  };
}

#endif
