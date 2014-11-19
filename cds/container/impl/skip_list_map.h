//$$CDS-header$$

#ifndef __CDS_CONTAINER_IMPL_SKIP_LIST_MAP_H
#define __CDS_CONTAINER_IMPL_SKIP_LIST_MAP_H

#include <cds/container/details/guarded_ptr_cast.h>

namespace cds { namespace container {

    /// Lock-free skip-list map
    /** @ingroup cds_nonintrusive_map
        \anchor cds_nonintrusive_SkipListMap_hp

        The implementation of well-known probabilistic data structure called skip-list
        invented by W.Pugh in his papers:
            - [1989] W.Pugh Skip Lists: A Probabilistic Alternative to Balanced Trees
            - [1990] W.Pugh A Skip List Cookbook

        A skip-list is a probabilistic data structure that provides expected logarithmic
        time search without the need of rebalance. The skip-list is a collection of sorted
        linked list. Nodes are ordered by key. Each node is linked into a subset of the lists.
        Each list has a level, ranging from 0 to 32. The bottom-level list contains
        all the nodes, and each higher-level list is a sublist of the lower-level lists.
        Each node is created with a random top level (with a random height), and belongs
        to all lists up to that level. The probability that a node has the height 1 is 1/2.
        The probability that a node has the height N is 1/2 ** N (more precisely,
        the distribution depends on an random generator provided, but our generators
        have this property).

        The lock-free variant of skip-list is implemented according to book
            - [2008] M.Herlihy, N.Shavit "The Art of Multiprocessor Programming",
                chapter 14.4 "A Lock-Free Concurrent Skiplist"

        Template arguments:
        - \p GC - Garbage collector used.
        - \p K - type of a key to be stored in the list.
        - \p T - type of a value to be stored in the list.
        - \p Traits - map traits, default is \p skip_list::traits
            It is possible to declare option-based list with \p cds::container::skip_list::make_traits metafunction 
            istead of \p Traits template argument.

        Like STL map class, \p %SkipListMap stores the key-value pair as <tt>std:pair< K const, T></tt>.

        @warning The skip-list requires up to 67 hazard pointers that may be critical for some GCs for which
            the guard count is limited (like \p gc::HP). Those GCs should be explicitly initialized with
            hazard pointer enough: \code cds::gc::HP myhp( 67 ) \endcode. Otherwise an run-time exception may be raised
            when you try to create skip-list object.

        @note There are several specializations of \p %SkipListMap for each \p GC. You should include:
        - <tt><cds/container/skip_list_map_hp.h></tt> for \p gc::HP garbage collector
        - <tt><cds/container/skip_list_map_dhp.h></tt> for \p gc::DHP garbage collector
        - <tt><cds/container/skip_list_map_rcu.h></tt> for \ref cds_nonintrusive_SkipListMap_rcu "RCU type"
        - <tt><cds/container/skip_list_map_nogc.h></tt> for \ref cds_nonintrusive_SkipListMap_nogc "non-deletable SkipListMap"

        <b>Iterators</b>

        The class supports a forward iterator (\ref iterator and \ref const_iterator).
        The iteration is ordered.
        The iterator object is thread-safe: the element pointed by the iterator object is guarded,
        so, the element cannot be reclaimed while the iterator object is alive.
        However, passing an iterator object between threads is dangerous.

        \warning Due to concurrent nature of skip-list map it is not guarantee that you can iterate
        all elements in the map: any concurrent deletion can exclude the element
        pointed by the iterator from the map, and your iteration can be terminated
        before end of the map. Therefore, such iteration is more suitable for debugging purpose only

        Remember, each iterator object requires 2 additional hazard pointers, that may be
        a limited resource for \p GC like \p gc::HP (for gc::DHP the count of
        guards is unlimited).

        The iterator class supports the following minimalistic interface:
        \code
        struct iterator {
            // Default ctor
            iterator();

            // Copy ctor
            iterator( iterator const& s);

            value_type * operator ->() const;
            value_type& operator *() const;

            // Pre-increment
            iterator& operator ++();

            // Copy assignment
            iterator& operator = (const iterator& src);

            bool operator ==(iterator const& i ) const;
            bool operator !=(iterator const& i ) const;
        };
        \endcode
        Note, the iterator object returned by \ref end, \ cend member functions points to \p nullptr and should not be dereferenced.

    */
    template <
        typename GC,
        typename Key,
        typename T,
#ifdef CDS_DOXYGEN_INVOKED
        typename Traits = skip_list::traits
#else
        typename Traits
#endif
    >
    class SkipListMap:
#ifdef CDS_DOXYGEN_INVOKED
        protected intrusive::SkipListSet< GC, std::pair<Key const, T>, Traits >
#else
        protected details::make_skip_list_map< GC, Key, T, Traits >::type
#endif
    {
        //@cond
        typedef details::make_skip_list_map< GC, Key, T, Traits > maker;
        typedef typename maker::type base_class;
        //@endcond
    public:
        typedef GC      gc;          ///< Garbage collector
        typedef Key     key_type;    ///< Key type
        typedef T       mapped_type; ///< Mapped type
        typedef Traits  traits;      ///< Map traits
#   ifdef CDS_DOXYGEN_INVOKED
        typedef std::pair< K const, T> value_type;   ///< Key-value pair to be stored in the map
#   else
        typedef typename maker::value_type  value_type;
#   endif

        typedef typename base_class::back_off      back_off;       ///< Back-off strategy
        typedef typename traits::allocator         allocator_type; ///< Allocator type used for allocate/deallocate the skip-list nodes
        typedef typename base_class::item_counter  item_counter;   ///< Item counting policy used
        typedef typename maker::key_comparator     key_comparator; ///< key comparison functor
        typedef typename base_class::memory_model  memory_model;   ///< Memory ordering, see \p cds::opt::memory_model
        typedef typename traits::random_level_generator random_level_generator ; ///< random level generator
        typedef typename traits::stat              stat;           ///< internal statistics type

    protected:
        //@cond
        typedef typename maker::node_type           node_type;
        typedef typename maker::node_allocator      node_allocator;

        typedef std::unique_ptr< node_type, typename maker::node_deallocator >    scoped_node_ptr;
        //@endcond

    public:
        /// Guarded pointer
        typedef typename gc::template guarded_ptr< node_type, value_type, details::guarded_ptr_cast_set<node_type, value_type> > guarded_ptr;

    protected:
        //@cond
        unsigned int random_level()
        {
            return base_class::random_level();
        }
        //@endcond

    public:
        /// Default ctor
        SkipListMap()
            : base_class()
        {}

        /// Destructor destroys the set object
        ~SkipListMap()
        {}

    public:
        /// Iterator type
        typedef skip_list::details::iterator< typename base_class::iterator >  iterator;

        /// Const iterator type
        typedef skip_list::details::iterator< typename base_class::const_iterator >   const_iterator;

        /// Returns a forward iterator addressing the first element in a map
        iterator begin()
        {
            return iterator( base_class::begin() );
        }

        /// Returns a forward const iterator addressing the first element in a map
        const_iterator begin() const
        {
            return cbegin();
        }
        /// Returns a forward const iterator addressing the first element in a map
        const_iterator cbegin() const
        {
            return const_iterator( base_class::cbegin() );
        }

        /// Returns a forward iterator that addresses the location succeeding the last element in a map.
        iterator end()
        {
            return iterator( base_class::end() );
        }

        /// Returns a forward const iterator that addresses the location succeeding the last element in a map.
        const_iterator end() const
        {
            return cend();
        }
        /// Returns a forward const iterator that addresses the location succeeding the last element in a map.
        const_iterator cend() const
        {
            return const_iterator( base_class::cend() );
        }

    public:
        /// Inserts new node with key and default value
        /**
            The function creates a node with \p key and default value, and then inserts the node created into the map.

            Preconditions:
            - The \p key_type should be constructible from a value of type \p K.
                In trivial case, \p K is equal to \p key_type.
            - The \p mapped_type should be default-constructible.

            Returns \p true if inserting successful, \p false otherwise.
        */
        template <typename K>
        bool insert( K const& key )
        {
            return insert_key( key, [](value_type&){} );
        }

        /// Inserts new node
        /**
            The function creates a node with copy of \p val value
            and then inserts the node created into the map.

            Preconditions:
            - The \p key_type should be constructible from \p key of type \p K.
            - The \p value_type should be constructible from \p val of type \p V.

            Returns \p true if \p val is inserted into the set, \p false otherwise.
        */
        template <typename K, typename V>
        bool insert( K const& key, V const& val )
        {
            return insert_key( key, [&val](value_type& item) { item.second = val ; } );
        }

        /// Inserts new node and initialize it by a functor
        /**
            This function inserts new node with key \p key and if inserting is successful then it calls
            \p func functor with signature
            \code
                struct functor {
                    void operator()( value_type& item );
                };
            \endcode

            The argument \p item of user-defined functor \p func is the reference
            to the map's item inserted:
                - <tt>item.first</tt> is a const reference to item's key that cannot be changed.
                - <tt>item.second</tt> is a reference to item's value that may be changed.

            \p key_type should be constructible from value of type \p K.

            The function allows to split creating of new item into two part:
            - create item from \p key;
            - insert new item into the map;
            - if inserting is successful, initialize the value of item by calling \p func functor

            This can be useful if complete initialization of object of \p value_type is heavyweight and
            it is preferable that the initialization should be completed only if inserting is successful.
        */
        template <typename K, typename Func>
        bool insert_key( const K& key, Func func )
        {
            scoped_node_ptr pNode( node_allocator().New( random_level(), key ));
            if ( base_class::insert( *pNode, [&func]( node_type& item ) { func( item.m_Value ); } )) {
                pNode.release();
                return true;
            }
            return false;
        }

        /// For key \p key inserts data of type \p value_type created in-place from <tt>std::forward<Args>(args)...</tt>
        /**
            Returns \p true if inserting successful, \p false otherwise.
        */
        template <typename K, typename... Args>
        bool emplace( K&& key, Args&&... args )
        {
            scoped_node_ptr pNode( node_allocator().New( random_level(), std::forward<K>(key), std::forward<Args>(args)... ));
            if ( base_class::insert( *pNode )) {
                pNode.release();
                return true;
            }
            return false;
        }

        /// Ensures that the \p key exists in the map
        /**
            The operation performs inserting or changing data with lock-free manner.

            If the \p key not found in the map, then the new item created from \p key
            is inserted into the map (note that in this case the \ref key_type should be
            constructible from type \p K).
            Otherwise, the functor \p func is called with item found.
            The functor \p Func may be a function with signature:
            \code
                void func( bool bNew, value_type& item );
            \endcode
            or a functor:
            \code
                struct my_functor {
                    void operator()( bool bNew, value_type& item );
                };
            \endcode

            with arguments:
            - \p bNew - \p true if the item has been inserted, \p false otherwise
            - \p item - item of the list

            The functor may change any fields of the \p item.second that is \ref value_type.

            Returns <tt> std::pair<bool, bool> </tt> where \p first is true if operation is successfull,
            \p second is true if new item has been added or \p false if the item with \p key
            already is in the list.

            @warning See \ref cds_intrusive_item_creating "insert item troubleshooting"
        */
        template <typename K, typename Func>
        std::pair<bool, bool> ensure( K const& key, Func func )
        {
            scoped_node_ptr pNode( node_allocator().New( random_level(), key ));
            std::pair<bool, bool> res = base_class::ensure( *pNode,
                [&func](bool bNew, node_type& item, node_type const& ){ func( bNew, item.m_Value ); }
            );
            if ( res.first && res.second )
                pNode.release();
            return res;
        }

        /// Delete \p key from the map
        /** \anchor cds_nonintrusive_SkipListMap_erase_val

            Return \p true if \p key is found and deleted, \p false otherwise
        */
        template <typename K>
        bool erase( K const& key )
        {
            return base_class::erase(key);
        }

        /// Deletes the item from the map using \p pred predicate for searching
        /**
            The function is an analog of \ref cds_nonintrusive_SkipListMap_erase_val "erase(K const&)"
            but \p pred is used for key comparing.
            \p Less functor has the interface like \p std::less.
            \p Less must imply the same element order as the comparator used for building the map.
        */
        template <typename K, typename Less>
        bool erase_with( K const& key, Less pred )
        {
            CDS_UNUSED( pred );
            return base_class::erase_with( key, cds::details::predicate_wrapper< node_type, Less, typename maker::key_accessor >());
        }

        /// Delete \p key from the map
        /** \anchor cds_nonintrusive_SkipListMap_erase_func

            The function searches an item with key \p key, calls \p f functor
            and deletes the item. If \p key is not found, the functor is not called.

            The functor \p Func interface:
            \code
            struct extractor {
                void operator()(value_type& item) { ... }
            };
            \endcode

            Return \p true if key is found and deleted, \p false otherwise
        */
        template <typename K, typename Func>
        bool erase( K const& key, Func f )
        {
            return base_class::erase( key, [&f]( node_type& node) { f( node.m_Value ); } );
        }

        /// Deletes the item from the map using \p pred predicate for searching
        /**
            The function is an analog of \ref cds_nonintrusive_SkipListMap_erase_func "erase(K const&, Func)"
            but \p pred is used for key comparing.
            \p Less functor has the interface like \p std::less.
            \p Less must imply the same element order as the comparator used for building the map.
        */
        template <typename K, typename Less, typename Func>
        bool erase_with( K const& key, Less pred, Func f )
        {
            CDS_UNUSED( pred );
            return base_class::erase_with( key,
                cds::details::predicate_wrapper< node_type, Less, typename maker::key_accessor >(),
                [&f]( node_type& node) { f( node.m_Value ); } );
        }

        /// Extracts the item from the map with specified \p key
        /** \anchor cds_nonintrusive_SkipListMap_hp_extract
            The function searches an item with key equal to \p key in the map,
            unlinks it from the map, and returns a guarded pointer to the item found.
            If \p key is not found the function returns an empty guarded pointer.

            Note the compare functor should accept a parameter of type \p K that can be not the same as \p key_type.

            The item extracted is freed automatically by garbage collector \p GC
            when returned \p guarded_ptr object will be destroyed or released.
            @note Each \p guarded_ptr object uses the GC's guard that can be limited resource.

            Usage:
            \code
            typedef cds::container::SkipListMap< cds::gc::HP, int, foo, my_traits >  skip_list;
            skip_list theList;
            // ...
            {
                skip_list::guarded_ptr gp( theList.extract( 5 ));
                if ( gp ) {
                    // Deal with gp
                    // ...
                }
                // Destructor of gp releases internal HP guard and frees the pointer
            }
            \endcode
        */
        template <typename K>
        guarded_ptr extract( K const& key )
        {
            guarded_ptr gp;
            base_class::extract_( gp.guard(), key, typename base_class::key_comparator() );
            return gp;
        }

        /// Extracts the item from the map with comparing functor \p pred
        /**
            The function is an analog of \ref cds_nonintrusive_SkipListMap_hp_extract "extract(K const&)"
            but \p pred predicate is used for key comparing.

            \p Less functor has the semantics like \p std::less but should take arguments of type \ref key_type and \p K
            in any order.
            \p pred must imply the same element order as the comparator used for building the map.
        */
        template <typename K, typename Less>
        guarded_ptr extract_with( K const& key, Less pred )
        {
            CDS_UNUSED( pred );
            typedef cds::details::predicate_wrapper< node_type, Less, typename maker::key_accessor >  wrapped_less;
            guarded_ptr gp;
            base_class::extract_( gp.guard(), key, cds::opt::details::make_comparator_from_less<wrapped_less>() );
            return gp;
        }

        /// Extracts an item with minimal key from the map
        /**
            The function searches an item with minimal key, unlinks it, and returns an guarded pointer to the item found.
            If the skip-list is empty the function returns an empty guarded pointer.

            The item extracted is freed automatically by garbage collector \p GC
            when returned \p guarded_ptr object will be destroyed or released.
            @note Each \p guarded_ptr object uses the GC's guard that can be limited resource.

            Usage:
            \code
            typedef cds::continer::SkipListMap< cds::gc::HP, int, foo, my_traits >  skip_list;
            skip_list theList;
            // ...
            {
                skip_list::guarded_ptr gp( theList.extract_min());
                if ( gp ) {
                    // Deal with gp
                    //...
                }
                // Destructor of gp releases internal HP guard and then frees the pointer
            }
            \endcode
        */
        guarded_ptr extract_min()
        {
            guarded_ptr gp;
            base_class::extract_min_( gp.guard() );
            return gp;
        }

        /// Extracts an item with maximal key from the map
        /**
            The function searches an item with maximal key, unlinks it, and returns a guarded pointer to item found.
            If the skip-list is empty the function returns an empty \p guarded_ptr.

            The item found is freed by garbage collector \p GC automatically
            when returned \p guarded_ptr object will be destroyed or released.
            @note Each \p guarded_ptr object uses the GC's guard that can be limited resource.

            Usage:
            \code
            typedef cds::container::SkipListMap< cds::gc::HP, int, foo, my_traits >  skip_list;
            skip_list theList;
            // ...
            {
                skip_list::guarded_ptr gp( theList.extract_max());
                if ( gp ) {
                    // Deal with gp
                    //...
                }
                // Destructor of gp releases internal HP guard and then frees the pointer
            }
            \endcode
        */
        guarded_ptr extract_max()
        {
            guarded_ptr gp;
            base_class::extract_max_( gp.guard() );
            return gp;
        }

        /// Find the key \p key
        /** \anchor cds_nonintrusive_SkipListMap_find_cfunc

            The function searches the item with key equal to \p key and calls the functor \p f for item found.
            The interface of \p Func functor is:
            \code
            struct functor {
                void operator()( value_type& item );
            };
            \endcode
            where \p item is the item found.

            The functor may change \p item.second.

            The function returns \p true if \p key is found, \p false otherwise.
        */
        template <typename K, typename Func>
        bool find( K const& key, Func f )
        {
            return base_class::find( key, [&f](node_type& item, K const& ) { f( item.m_Value );});
        }

        /// Finds the key \p val using \p pred predicate for searching
        /**
            The function is an analog of \ref cds_nonintrusive_SkipListMap_find_cfunc "find(K const&, Func)"
            but \p pred is used for key comparing.
            \p Less functor has the interface like \p std::less.
            \p Less must imply the same element order as the comparator used for building the map.
        */
        template <typename K, typename Less, typename Func>
        bool find_with( K const& key, Less pred, Func f )
        {
            CDS_UNUSED( pred );
            return base_class::find_with( key,
                cds::details::predicate_wrapper< node_type, Less, typename maker::key_accessor >(),
                [&f](node_type& item, K const& ) { f( item.m_Value );});
        }

        /// Find the key \p key
        /** \anchor cds_nonintrusive_SkipListMap_find_val

            The function searches the item with key equal to \p key
            and returns \p true if it is found, and \p false otherwise.
        */
        template <typename K>
        bool find( K const& key )
        {
            return base_class::find( key );
        }

        /// Finds the key \p val using \p pred predicate for searching
        /**
            The function is an analog of \ref cds_nonintrusive_SkipListMap_find_val "find(K const&)"
            but \p pred is used for key comparing.
            \p Less functor has the interface like \p std::less.
            \p Less must imply the same element order as the comparator used for building the map.
        */
        template <typename K, typename Less>
        bool find_with( K const& key, Less pred )
        {
            CDS_UNUSED( pred );
            return base_class::find_with( key, cds::details::predicate_wrapper< node_type, Less, typename maker::key_accessor >() );
        }

        /// Finds the key \p key and return the item found
        /** \anchor cds_nonintrusive_SkipListMap_hp_get
            The function searches the item with key equal to \p key
            and returns an guarded pointer to the item found.
            If \p key is not found the function returns an empty guarded pointer.

            It is safe when a concurrent thread erases the item returned as \p guarded_ptr.
            In this case the item will be freed later by garbage collector \p GC automatically
            when \p guarded_ptr object will be destroyed or released.
            @note Each \p guarded_ptr object uses one GC's guard which can be limited resource.

            Usage:
            \code
            typedef cds::container::SkipListMap< cds::gc::HP, int, foo, my_traits >  skip_list;
            skip_list theList;
            // ...
            {
                skip_list::guarded_ptr gp( theList.get( 5 ));
                if ( gp ) {
                    // Deal with gp
                    //...
                }
                // Destructor of guarded_ptr releases internal HP guard
            }
            \endcode

            Note the compare functor specified for class \p Traits template parameter
            should accept a parameter of type \p K that can be not the same as \p value_type.
        */
        template <typename K>
        guarded_ptr get( K const& key )
        {
            guarded_ptr gp;
            base_class::get_with_( gp.guard(), key, typename base_class::key_comparator() );
            return gp;
        }

        /// Finds the key \p key and return the item found
        /**
            The function is an analog of \ref cds_nonintrusive_SkipListMap_hp_get "get( K const&)"
            but \p pred is used for comparing the keys.

            \p Less functor has the semantics like \p std::less but should take arguments of type \ref key_type and \p K
            in any order.
            \p pred must imply the same element order as the comparator used for building the map.
        */
        template <typename K, typename Less>
        guarded_ptr get_with( K const& key, Less pred )
        {
            CDS_UNUSED( pred );
            typedef cds::details::predicate_wrapper< node_type, Less, typename maker::key_accessor > wrapped_less;
            guarded_ptr gp;
            base_class::get_with_( gp.guard(), key, cds::opt::details::make_comparator_from_less< wrapped_less >());
            return gp;
        }

        /// Clears the map
        void clear()
        {
            base_class::clear();
        }

        /// Checks if the map is empty
        bool empty() const
        {
            return base_class::empty();
        }

        /// Returns item count in the map
        size_t size() const
        {
            return base_class::size();
        }

        /// Returns const reference to internal statistics
        stat const& statistics() const
        {
            return base_class::statistics();
        }
    };
}} // namespace cds::container

#endif // #ifndef __CDS_CONTAINER_IMPL_SKIP_LIST_MAP_H
