#ifndef SOFTDSP_CONSTANT_GENERATOR_HPP
#define SOFTDSP_CONSTANT_GENERATOR_HPP

#include <llvm/ADT/ArrayRef.h>
#include <llvm/LLVMContext.h>
#include <llvm/Constants.h>
#include <llvm/Type.h>
#include <llvm/DerivedTypes.h>
#include <array>
#include <memory>
#include <vector>
#include <boost/array.hpp>
#include <boost/range/algorithm.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/type_traits.hpp>
#include <boost/fusion/algorithm.hpp>
#include <boost/fusion/include/boost_array.hpp>
#include <boost/fusion/include/copy.hpp>
#include <boost/fusion/support/is_sequence.hpp>
#include <boost/fusion/include/make_vector.hpp>
#include <boost/fusion/adapted/adt/adapt_adt.hpp>
#include <boost/fusion/mpl.hpp>

#include <hermit/range_traits.hpp>
#include <softdsp/static_range_size.hpp>

#include <cxxabi.h>


namespace softdsp {
  class constant_generator {
  public:
    typedef llvm::Constant *result_type;
    constant_generator( const std::shared_ptr< llvm::LLVMContext > &context_ ) : context( context_ ) {}
    llvm::Constant *operator()(
      bool value
    ) const {
      if( value )
        return llvm::ConstantInt::getTrue( *context );
      else
        return llvm::ConstantInt::getFalse( *context );
    }
    template< typename T >
    llvm::Constant *operator()(
      T value,
      typename boost::enable_if<
        boost::mpl::and_<
          boost::mpl::not_< boost::is_same< boost::remove_cv< T >, bool > >,
          boost::is_integral< T >
        >
      >::type* = 0
    ) const {
      return llvm::ConstantInt::get( llvm::IntegerType::get( *context, sizeof( T ) * 8 ), value, boost::is_signed< T >::value );
    }
    llvm::Constant *operator()(
      float value
    ) const {
      return llvm::ConstantFP::get( llvm::Type::getFloatTy( *context ), value );
    }
    llvm::Constant *operator()(
      double value
    ) const {
      return llvm::ConstantFP::get( llvm::Type::getDoubleTy( *context ), value );
    }
    template< typename Sequence >
    llvm::Constant *operator()(
      const Sequence &tuple,
      typename boost::enable_if<
        boost::fusion::traits::is_sequence< Sequence >
      >::type* = 0
    ) const {
      boost::array< llvm::Constant*, boost::fusion::result_of::size< Sequence >::value > elements_as_constant_;
      boost::fusion::copy(
        boost::fusion::transform(
          tuple,
          *this
        ),
        elements_as_constant_
      );
      std::vector< llvm::Constant* > elements_as_constant( elements_as_constant_.begin(), elements_as_constant_.end() );
      llvm::ArrayRef< llvm::Constant* > ref(  elements_as_constant );
      return llvm::ConstantStruct::get(
        llvm::ConstantStruct::getTypeForElements( *context, ref, false ),
        ref
      );
    }
    template< typename Range >
    llvm::Constant *operator()(
      const Range &range,
      typename boost::enable_if<
        boost::mpl::and_<
          hermit::is_forward_traversal_range< Range >,
          hermit::is_readable_range< Range >
        >
      >::type* = 0
    ) const {
      std::vector< llvm::Constant* > elements_as_constant;
      boost::transform(
        range,
        std::back_inserter( elements_as_constant ),
        *this
      );
      llvm::ArrayRef< llvm::Constant* > ref( elements_as_constant );
      return llvm::ConstantArray::get( llvm::ArrayType::get( elements_as_constant.front()->getType(), elements_as_constant.size() ), ref );
    }
  private:
    const std::shared_ptr< llvm::LLVMContext > context;
  };
}

#endif
