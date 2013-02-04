#ifndef SOFTDSP_DATA_LAYOUT_TUPLE_HPP
#define SOFTDSP_DATA_LAYOUT_TUPLE_HPP

#include <softdsp/tag.hpp>
#include <softdsp/data_layout/common.hpp>
#include <softdsp/data_layout/get_element_type.hpp>
#include <llvm/LLVMContext.h>
#include <llvm/DataLayout.h>
#include <memory>
#include <boost/range/iterator_range.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/type_traits.hpp>
#include <boost/fusion/algorithm.hpp>
#include <boost/fusion/include/copy.hpp>
#include <boost/fusion/support/is_sequence.hpp>
#include <boost/fusion/mpl.hpp>

namespace softdsp {
  namespace data_layout {
    template< typename head, typename ...tail >
    struct get_internal_fusion_sequence {
      typedef
        typename boost::mpl::push_front<
          typename get_internal_fusion_sequence<
            tail...
          >::type,
          typename get_element_type< head >::type
        >::type type;
    };
    
    template< typename head >
    struct get_internal_fusion_sequence< head > {
      typedef boost::fusion::vector<
        typename get_element_type< head >::type
      > type;
    };

    template< typename T >
    T construct_internal_container(
      const std::shared_ptr< common > &common_resources_,
      const boost::iterator_range< uint8_t* > range_,
      const llvm::StructLayout *struct_layout,
      size_t size,
      size_t index = 0,
      typename boost::enable_if<
        boost::mpl::not_< typename boost::fusion::result_of::empty< T >::type >
      >::type* = 0
    ) {
      const auto element_range = boost::iterator_range<uint8_t*>(
        boost::begin( range_ ) + struct_layout->getElementOffset( size - index - 1 ),
        boost::begin( range_ ) + struct_layout->getElementOffset( size - index - 1 ) +
        common_resources_->layout.getTypeStoreSize(
          common_resources_->type_generator_( tag< typename boost::mpl::back< T >::type >() )
        ) 
      );
      return boost::fusion::push_back(
        construct_internal_container< typename boost::mpl::pop_back< T >::type >(
          common_resources_,
          range_,
          struct_layout,
          size,
          index + 1
        ),
        typename boost::mpl::back< T >::type(
          common_resources_, element_range
        )
      );
    }

    template< typename T >
    T construct_internal_container(
      const std::shared_ptr< common > &,
      const boost::iterator_range< uint8_t* >&,
      const llvm::StructLayout*,
      size_t,
      size_t = 0,
      typename boost::enable_if<
        typename boost::fusion::result_of::empty< T >::type
      >::type* = 0
    ) {
      return boost::fusion::vector<>();
    }

    template< typename T >
    T construct_internal_container(
      const std::shared_ptr< llvm::LLVMContext > &context_,
      const std::string &layout_
    ) {
      const std::shared_ptr< common > common_resources_(
        new common( context_, layout_, tag< T >() )
      );
      return construct_internal_container< T >(
        common_resources_,
        common_resources_->root,
        common_resources_->layout.getStructLayout(
          common_resources_->type_generator_(
            tag< T >()
          )
        ),
        boost::mpl::size< T >::value
      );
    }

    template< typename ...Args >
    class tuple : public get_internal_fusion_sequence< Args... >::type {
    public:
      typedef typename get_internal_fusion_sequence< Args... >::type internal_container; 
      tuple(
        const std::shared_ptr< common > common_resources_,
        const boost::iterator_range< uint8_t* > range_
      ) : internal_container(
          construct_internal_container< internal_container >(
            common_resources_,
            range_,
            common_resources_->layout.getStructLayout(
              common_resources_->type_generator_(
                tag< boost::fusion::vector< Args... > >()
              )
            ),
            boost::mpl::size< boost::mpl::vector< Args... > >::value
          )
        ), head( boost::begin( range_ ) ) {
      }
      const uint8_t * get() const {
        return head;
      }
    private:
      const uint8_t * const head;
    };
  }
}

#endif
