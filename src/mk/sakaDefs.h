//
//  sakaDefs.h
//  SakaSVG
//
//  Created by Michel Donais on 17-05-22.
//

#ifndef sakaDefs_h
#define sakaDefs_h

#include <boost/pool/pool_alloc.hpp>
#include <vector>
#include <list>
#include <deque>
#include <map>
#include <set>
#include <unordered_map>
#include <cpp_btree/btree_map.h>
#include <scoped_allocator>


namespace Saka {
    template <typename T>
    using fast_pool_allocator = boost::fast_pool_allocator<T, boost::default_user_allocator_new_delete, boost::details::pool::null_mutex, 1024>;
    
    template <typename T>
    using vector = std::vector<T>;

    template <typename T>
    using list = std::list<T, fast_pool_allocator<T>>;

    template <typename T>
    using deque = std::deque<T, fast_pool_allocator<T>>;

    template <typename T>
    using set = std::set<T, fast_pool_allocator<T>>;

    template <typename T, typename U>
    using map = std::map<T, U, std::less<T>, fast_pool_allocator<std::pair<const T, U>>>;
    
    namespace _unordered_map
    {
        template <typename T>
        using stdAlloc = std::allocator<T>;
        
        template <typename T>
        using fastAlloc = fast_pool_allocator<T>;
        
        
        // This is our data type and its allocator. It's the simplest one
        template <typename T, typename U>
        using data_type = std::pair<const T, U>;
        
        template <typename T, typename U>
        using data_type_allocator = stdAlloc<data_type<T, U>>;

        // We must specifically instantiate every single rebind so we don't miss any
        template <typename T, typename U, typename Up> struct rebind_selector : public std::false_type {};

        template <typename T, typename U, typename Up> struct rebind_selector_checked : public rebind_selector<T, U, Up>
        {
            static_assert(rebind_selector<T, U, Up>::value, "Rebind not valid. Specialize rebind_selector");
        };
        
        // Our allocator, it sets itself as the data_type_allocator by default, and rebinds everything underneath
        template <typename T, typename U, typename AsA = data_type_allocator<T, U> >
        class allocator : public AsA
        {
        public:
            template<typename Up>
            using rebind = rebind_selector_checked<T, U, Up>;
        };
        
        // These are all the overrides required for the rebind of the current version of std::unordered_map. Fun, isn't it?
        template<typename T, typename U>
        struct rebind_selector<T, U, data_type<T, U>> : public std::true_type {using other = allocator<T, U, data_type_allocator<T,U>>;};
        

        template <typename T, typename U>
        using hash_value_type = typename std::__hash_value_type<T, U>;
        
        template <typename T, typename U>
        using hash_value_type_allocator = fastAlloc<hash_value_type<T, U>>;

        template<typename T, typename U>
        struct rebind_selector<T, U, hash_value_type<T, U>> : public std::true_type {using other = allocator<T, U, hash_value_type_allocator<T,U>>;};
        

        template <typename T, typename U>
        using hash_node_type = typename std::__make_hash_node_types<U, typename std::allocator_traits<data_type_allocator<T, U>>::void_pointer>::type::__node_type;
        
        template <typename T, typename U>
        using hash_node_type_allocator = fastAlloc<hash_node_type<T, U>>;
        
        template<typename T, typename U>
        struct rebind_selector<T, U, hash_node_type<T, U>> : public std::true_type {using other = allocator<T, U, hash_node_type_allocator<T,U>>;};
        
        
        template <typename T, typename U>
        using hash_node = typename std::__hash_node<hash_value_type<T, U>, typename std::allocator_traits<data_type_allocator<T, U>>::void_pointer>;
        
        template <typename T, typename U>
        using hash_node_allocator = fastAlloc<hash_node<T, U>>;

        template<typename T, typename U>
        struct rebind_selector<T, U, hash_node<T, U>> : public std::true_type {using other = allocator<T, U, hash_node_allocator<T,U>>;};


        template <typename T, typename U>
        using hash_node_base = typename std::__hash_node_base<hash_node<T, U>*>;
        
        template <typename T, typename U>
        using hash_node_base_allocator = fastAlloc<hash_node_base<T, U>>;

        template <typename T, typename U>
        using hash_node_base_allocator_ptr = stdAlloc<hash_node_base<T, U>*>;   // This one gets reserved. Keep it as normal allocator

        template<typename T, typename U>
        struct rebind_selector<T, U, hash_node_base<T, U>> : public std::true_type {using other = allocator<T, U, hash_node_base_allocator<T, U>>;};

        template<typename T, typename U>
        struct rebind_selector<T, U, hash_node_base<T, U>*> : public std::true_type {using other = allocator<T, U, hash_node_base_allocator_ptr<T, U>>;};
    }
    template <typename T, typename U>
    using unordered_map = std::unordered_map<T, U, std::hash<T>, std::equal_to<T>, _unordered_map::allocator<T, U>>;
    
    template <typename T, typename U>
    using btree_multimap = btree::btree_multimap<T, U>;
}


#endif /* mkDefs_h */
