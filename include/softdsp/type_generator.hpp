#ifndef SOFTDSP_TYPE_GENERATOR_HPP
#define SOFTDSP_TYPE_GENERATOR_HPP

#include <llvm/ADT/ArrayRef.h>
#include <llvm/LLVMContext.h>
#include <llvm/Module.h>
#include <llvm/Function.h>
#include <llvm/BasicBlock.h>
#include <llvm/Support/IRBuilder.h>
#include <llvm/DataLayout.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Support/TargetRegistry.h>
#include <array>
#include <memory>
#include <vector>
#include <string>
#include <boost/range/algorithm.hpp>
#include <boost/range/reference.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/type_traits.hpp>
#include <boost/bind.hpp>
#include <boost/array.hpp>
#include <boost/fusion/algorithm.hpp>
#include <boost/fusion/include/boost_array.hpp>
#include <boost/fusion/include/copy.hpp>
#include <boost/fusion/support/is_sequence.hpp>
#include <boost/fusion/include/make_vector.hpp>
#include <boost/fusion/adapted/adt/adapt_adt.hpp>
#include <boost/fusion/mpl.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/karma.hpp>
#include <boost/preprocessor/enum.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/tuple/enum.hpp>
#include <boost/preprocessor/tuple/to_seq.hpp>
#include <boost/preprocessor/seq/transform.hpp>
#include <boost/preprocessor/repeat.hpp>
#include <boost/swap.hpp>

#include <hermit/range_traits.hpp>
#include <softdsp/static_range_size.hpp>
#include <softdsp/primitive.hpp>
#include <softdsp/tag.hpp>

#include <cxxabi.h>
namespace softdsp {
  namespace data_layout {
    template< typename T >
    class proxy;
  }

  class type_generator {
  public:
    type_generator( const std::shared_ptr< llvm::LLVMContext > &context_ ) : context( context_ ) {}
    llvm::IntegerType *operator()( tag< bool > ) const {
      return llvm::IntegerType::get( *context, 1 );
    }
    template< typename T >
    llvm::IntegerType *operator()(
      tag< T >,
      typename boost::enable_if<
        boost::mpl::and_<
          boost::mpl::not_< boost::is_same< boost::remove_cv< T >, bool > >,
          boost::is_integral< T >
        >
      >::type* = 0
    ) const {
      return llvm::IntegerType::get( *context, sizeof( T ) * 8 );
    }
    llvm::Type *operator()(
      tag< float >
    ) const {
      return llvm::Type::getFloatTy( *context );
    }
    llvm::Type *operator()(
      tag< double >
    ) const {
      return llvm::Type::getDoubleTy( *context );
    }
    template< typename Sequence >
    llvm::StructType *operator()(
      tag< Sequence >,
      typename boost::enable_if<
        boost::mpl::and_<
          boost::mpl::not_< hermit::is_forward_traversal_range< Sequence > >,
          boost::fusion::traits::is_sequence< Sequence >
        >
      >::type* = 0
    ) const {
      std::vector< llvm::Type* > types;
      transform_struct<
        typename boost::mpl::begin< Sequence >::type,
        typename boost::mpl::end< Sequence >::type
      >( types );
      llvm::ArrayRef< llvm::Type* > ref( types );
      return llvm::StructType::create( *context, ref );
    }
    template< typename Range >
    llvm::VectorType *operator()(
      tag< Range >,
      typename boost::enable_if<
        boost::mpl::and_<
          hermit::is_forward_traversal_range< Range >,
          softdsp::is_static_range< Range >,
          is_primitive< typename softdsp::static_range_size< Range >::type >
        >
      >::type* = 0
    ) const {
      return llvm::VectorType::get(
        (*this)(
          tag< typename boost::range_value< Range >::type >()
        ),
        softdsp::static_range_size< Range >::value
      );
    }
    template< typename Range >
    llvm::ArrayType *operator()(
      tag< Range >,
      typename boost::enable_if<
        boost::mpl::and_<
          hermit::is_forward_traversal_range< Range >,
          softdsp::is_static_range< Range >,
          boost::mpl::not_< is_primitive< typename softdsp::static_range_size< Range >::type > >
        >
      >::type* = 0
    ) const {
      return llvm::ArrayType::get(
        (*this)(
          tag< typename boost::range_value< Range >::type >()
        ),
        softdsp::static_range_size< Range >::value
      );
    }
    template< typename T >
    llvm::Type *operator()(
      tag< data_layout::proxy< T > > 
    ) const {
      return (*this)( tag< T >() );
    }
  private:
    template< typename begin, typename end >
    void transform_struct(
      std::vector< llvm::Type* > &dest,
      typename boost::disable_if<
        boost::is_same< begin, end >
      >::type* = 0
    ) const {
      std::cout << typeid( typename begin::type ).name() << std::endl;
      dest.push_back( (*this)( tag< typename begin::type >() ) );
      transform_struct< typename boost::mpl::next< begin >::type, end >( dest );
    }
    template< typename begin, typename end >
    void transform_struct(
      std::vector< llvm::Type* > &,
      typename boost::enable_if<
        boost::is_same< begin, end >
      >::type* = 0
    ) const {}
    const std::shared_ptr< llvm::LLVMContext > context;
  };
}

#endif

