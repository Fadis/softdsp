#ifndef SOFTDSP_DATA_LAYOUT_COMMON_HPP
#define SOFTDSP_DATA_LAYOUT_COMMON_HPP

#include <softdsp/type_generator.hpp>

#include <llvm/LLVMContext.h>
#include <llvm/DataLayout.h>
#include <memory>
#include <vector>

#include <softdsp/type_generator.hpp>

namespace softdsp {
  namespace data_layout {
    uint8_t *get_aligned_head( uint8_t *head, size_t alignment ) {
      const auto drift = reinterpret_cast<intptr_t>( head ) % alignment;
      return head + ( alignment - drift );
    }

    struct common {
      common(
        const std::shared_ptr< llvm::LLVMContext > &context_,
        const std::string &layout_,
        size_t size_
      ) :
        context( context_ ), layout( layout_ ), type_generator_( context ), data( size_ ) {}
      template< typename T >
      common(
        const std::shared_ptr< llvm::LLVMContext > &context_,
        const std::string &layout_,
        const tag< T > &tag_,
        typename boost::enable_if<
          boost::mpl::not_<
            boost::mpl::and_<
              hermit::is_forward_traversal_range< T >,
              boost::fusion::traits::is_sequence< T >
            >
          >
        >::type* = 0
      ) :
        context( context_ ),
        layout( layout_ ),
        type_generator_( context ),
        data(
          layout.getTypeAllocSize(
            type_generator_( tag_ )
          )
        ),
        root(
          data.data(),
          data.data() + data.size()
        ) {}
      template< typename T >
      common(
        const std::shared_ptr< llvm::LLVMContext > &context_,
        const std::string &layout_,
        const tag< T > &tag_,
        typename boost::enable_if<
          boost::mpl::and_<
            hermit::is_forward_traversal_range< T >,
            boost::fusion::traits::is_sequence< T >
          >
        >::type* = 0
      ) :
        context( context_ ),
        layout( layout_ ),
        type_generator_( context ),
        data(
          layout.getTypeAllocSize(
            type_generator_( tag_ )
          ) +
          layout.getStructLayout(
            type_generator_( tag_ )
          ).getAlignment()
        ),
        root(
          get_aligned_head(
            data.data(),
            layout.getStructLayout(
              type_generator_( tag_ )
            ).getAlignment()
          ),
          data.data() + data.size()
        ) {}
      const std::shared_ptr< llvm::LLVMContext > context;
      const llvm::DataLayout layout;
      const type_generator type_generator_; 
      std::vector< uint8_t > data;
      const boost::iterator_range< uint8_t* > root;
    };
  }
}

#endif
