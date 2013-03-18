#ifndef SOFTDSP_ARGUMENT_GENERATOR_HPP
#define SOFTDSP_ARGUMENT_GENERATOR_HPP

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
#include <boost/range/combine.hpp>
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
#include <softdsp/value_generator.hpp>
#include <softdsp/data_layout/proxy.hpp>
#include <softdsp/data_layout/array.hpp>
#include <softdsp/data_layout/tuple.hpp>

#include <cxxabi.h>


namespace softdsp {
  template< typename T, typename Enable = void >
  struct converted_type {
  };

  template< typename T >
  struct converted_type< T, typename boost::enable_if< is_primitive< T > >::type > {
    typedef data_layout::proxy< T > type;
  };
  
  template< typename T >
  struct converted_type< T, typename boost::enable_if< hermit::is_forward_traversal_range< T > >::type > {
    typedef data_layout::array< typename remove_proxy< typename converted_type< typename boost::range_value< T >::type >::type >::type, static_range_size< T >::value > type;
  };

  template< typename Sequence, size_t size = boost::fusion::result_of::size< Sequence >::value >
  struct to_tuple  {};

#define SOFTDSP_TO_TUPLE_ELEMENT( z, index, seq ) \
  typename remove_proxy< typename converted_type< typename boost::mpl::at_c< seq, index >::type >::type >::type

#define SOFTDSP_TO_TUPLE_SPEC( z, size, unused ) \
  template< typename Sequence > \
  struct to_tuple< Sequence, size > { \
    typedef data_layout::tuple< \
      BOOST_PP_ENUM( size, SOFTDSP_TO_TUPLE_ELEMENT, Sequence ) \
    > type; \
  };
BOOST_PP_REPEAT( FUSION_MAX_VECTOR_SIZE, SOFTDSP_TO_TUPLE_SPEC, )

  template< typename T >
  struct converted_type< T, typename boost::enable_if<
    boost::mpl::and_<
      boost::fusion::traits::is_sequence< T >,
      boost::mpl::not_< hermit::is_forward_traversal_range< T > >
    >
  >::type > {
    typedef typename to_tuple< T >::type type;
  };

  template< typename T, typename U >
  void intrusive_copy(
    const T &src,
    U &dest,
    typename boost::enable_if<
      boost::mpl::and_<
        is_primitive< typename remove_proxy< T >::type >,
        is_primitive< typename remove_proxy< U >::type >
      >
    >::type* = 0
  ) {
    dest = src;
  }
  
  template< typename T, typename U >
  void intrusive_copy(
    const T &src,
    U &dest,
    typename boost::enable_if<
      boost::mpl::and_<
        hermit::is_forward_traversal_range< typename remove_proxy< T >::type >,
        hermit::is_forward_traversal_range< typename remove_proxy< U >::type >
      >
    >::type* = 0
  ) {
    BOOST_FOREACH( auto &elem, boost::combine( src BOOST_PP_COMMA() dest ) )
      intrusive_copy( boost::fusion::at_c< 0 >( elem ), boost::fusion::at_c< 1 >( elem ) );
  }

  template< typename T, typename U, typename Index = boost::fusion::result_of::size< T > >
  void intrusive_copy(
    const T &src,
    U &dest,
    typename boost::enable_if<
      boost::mpl::and_<
        boost::mpl::not_< hermit::is_forward_traversal_range< typename remove_proxy< T >::type > >,
        boost::mpl::not_< hermit::is_forward_traversal_range< typename remove_proxy< U >::type > >,
        boost::fusion::traits::is_sequence< typename remove_proxy< T >::type >,
        boost::fusion::traits::is_sequence< typename remove_proxy< U >::type >,
        boost::mpl::not_equal_to< boost::mpl::size_t< 0 >, Index >
      >
    >::type* = 0
  ) {
    intrusive_copy( boost::fusion::at_c< Index::value - 1 >( src ), boost::fusion::at_c< Index::value - 1 >( dest ) );
    intrusive_copy< T, U, boost::mpl::minus< Index, boost::mpl::size_t< 1 > > >( src, dest );
  }
  
  template< typename T, typename U, typename Index = boost::fusion::result_of::size< T > >
  void intrusive_copy(
    const T &src,
    U &dest,
    typename boost::enable_if<
      boost::mpl::and_<
        boost::mpl::not_< hermit::is_forward_traversal_range< typename remove_proxy< T >::type > >,
        boost::mpl::not_< hermit::is_forward_traversal_range< typename remove_proxy< U >::type > >,
        boost::fusion::traits::is_sequence< typename remove_proxy< T >::type >,
        boost::fusion::traits::is_sequence< typename remove_proxy< U >::type >,
        boost::mpl::equal_to< boost::mpl::size_t< 0 >, Index >
      >
    >::type* = 0
  ) {}
/*
  class argument_generator {
  public:
    argument_generator(
      const std::shared_ptr< llvm::LLVMContext > &context_,
      const std::string &layout_
    ) : context( context_ ), layout( layout_ ) {}
    template< typename T >
    typename converted_type< T >::type  operator()(
      const T &src,
      typename boost::enable_if<
        is_primitive< T >
      >::type* = 0
    ) {
      dest = src;
      return dest;
    }
  private:
    typename converted_type< T >::type operator()(
    ) {
      const std::shared_ptr< data_layout::common > common_resources_(
        new data_layout::common( context, layout, tag< typename converted_type< T >::type >() )
      );
      T dest(
        common_resources_,
        boost::iterator_range< uint8_t* >(
          common_resources_->data.data(),
          common_resources_->data.data() + common_resources_->data.size() 
        )
      );
    }
    const std::shared_ptr< llvm::LLVMContext > context;
    const std::string layout;
  };

  template< typename T >
  typename converted_type< T >::type convert(
    const T &src,
    typename boost::enable_if<
      is_primitive< T >
    >::type* = 0
  ) {
    const auto value_generator<  >;
    return dest;
  }
  */
}

#endif
