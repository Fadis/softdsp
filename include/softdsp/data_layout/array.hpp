#ifndef SOFTDSP_DATA_LAYOUT_ARRAY_HPP
#define SOFTDSP_DATA_LAYOUT_ARRAY_HPP

#include <softdsp/primitive.hpp>
#include <softdsp/tag.hpp>
#include <softdsp/data_layout/common.hpp>
#include <softdsp/data_layout/get_element_type.hpp>

#include <llvm/LLVMContext.h>
#include <llvm/Type.h>
#include <memory>
#include <vector>
#include <boost/range/algorithm.hpp>

namespace softdsp {
  namespace data_layout {
    template< typename T, size_t size_ >
    class array {
    public:
      typedef std::vector< typename get_element_type< T >::type > internal_container;
      typedef typename internal_container::value_type value_type;
      typedef typename internal_container::size_type size_type;
      typedef typename internal_container::difference_type difference_type;
      typedef typename internal_container::reference reference;
      typedef typename internal_container::const_reference const_reference;
      typedef typename internal_container::pointer pointer;
      typedef typename internal_container::const_pointer const_pointer;
      typedef typename internal_container::iterator iterator;
      typedef typename internal_container::const_iterator const_iterator;
      typedef typename internal_container::reverse_iterator reverse_iterator;
      typedef typename internal_container::const_reverse_iterator const_reverse_iterator;
      array(
        const std::shared_ptr< common > &common_resources_,
        const boost::iterator_range< uint8_t* > &range_
      ) : container( construct_internal_container( common_resources_, range_ ) ) {}
      array(
        const std::shared_ptr< llvm::LLVMContext > &context_,
        const std::string &layout_
      ) : container( construct_internal_container( context_, layout_ ) )
      {
      }
      array(
        const array< T, size_ > &src
      ) : container( src.container ) {
      }
      array(
        array< T, size_ > &&src
      ) : container( src.container ) {
      }
      reference at( size_type index ) {
        return container.at( index );
      }
      const_reference at( size_type index ) const {
        return container.at( index );
      }
      reference operator[]( size_type index ) {
        return container[ index ];
      }
      const_reference operator[]( size_type index ) const {
        return container[ index ];
      }
      reference front() {
        return container.front();
      }
      const_reference front() const {
        return container.front();
      }
      reference back() {
        return container.back();
      }
      const_reference back() const {
        return container.back();
      }
      iterator begin() {
        return container.begin();
      }
      const_iterator begin() const {
        return container.begin();
      }
      const_iterator cbegin() const {
        return container.cbegin();
      }
      iterator end() {
        return container.end();
      }
      const_iterator end() const {
        return container.end();
      }
      const_iterator cend() const {
        return container.end();
      }
      reverse_iterator rbegin() {
        return container.rbegin();
      }
      const_reverse_iterator rbegin() const {
        return container.rbegin();
      }
      const_reverse_iterator crbegin() const {
        return container.crbegin();
      }
      reverse_iterator rend() {
        return container.rend();
      }
      const_reverse_iterator rend() const {
        return container.rend();
      }
      const_reverse_iterator crend() const {
        return container.crend();
      }
      constexpr bool empty() {
        return size_ == 0u;
      }
      constexpr size_type size() {
        return size_;
      }
      constexpr size_type max_size() {
        return size_;
      }
      void swap( array< T, size_ > &other ) {
        boost::swap( container, other.container );
      }
    private:
      static internal_container construct_internal_container(
        const std::shared_ptr< common > &common_resources_,
        const boost::iterator_range< uint8_t* > &range_
      ) {
        internal_container container_;
        llvm::Type * element_type =
            common_resources_->type_generator_( tag< T >() );
        for( size_t index = 0; index != size_; ++index )
          container_.emplace_back(
            common_resources_,
            boost::iterator_range< uint8_t* >(
              boost::begin( range_ ) +
                common_resources_->layout.getTypeAllocSize( element_type ) * index,
              boost::begin( range_ ) +
                common_resources_->layout.getTypeAllocSize( element_type ) * index +
                common_resources_->layout.getTypeStoreSize( element_type )
            )
          );
        container_.shrink_to_fit();
        return container_;
      }
      static internal_container construct_internal_container(
        const std::shared_ptr< llvm::LLVMContext > &context_,
        const std::string &layout_
      ) {
        const std::shared_ptr< common > common_resources_(
          new common( context_, layout_, tag< array< T, size_ > >() )
        );
        return construct_internal_container(
          common_resources_,
          boost::iterator_range< uint8_t* >(
            common_resources_->data.data(),
            common_resources_->data.data() + common_resources_->data.size() 
          )
        );
      }
      internal_container container; 
    };
    template< typename T, size_t size_ >
    boost::mpl::size_t< size_ > get_static_range_size( array< T, size_ > );
  }
}

#endif
