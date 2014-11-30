/*
\section{Generic Resizable Array}

"Array.h" provides a generic resizable array. These arrays can be
stack allocated as long as you are careful that they do not get copied
excessively.  (One reason for stack allocation of arrays is so that
you can use the nice [] syntactic sugar.)

Arrays grow and shrink at the high end.  The low end always has index 0.

// \subsection{Constructor Specifications}


    Array()
	effects  - Creates empty array

    Array(int predict)
	requires - predict >= 0
	effects  - Creates empty array.  Extra storage is preallocated
		   under the assumption that the array will be grown
		   to contain predict elements.  This constructor form
		   helps avoids unnecessary copying.

    Array(T const* x, int count)
	requires - count >= 0 and x is a pointer to at least count elements.
	effects  - creates an array with a copy of the count elements pointed
		   to by x.  This constructor is useful for initializing an
		   array from a builtin array.

    Array(Array const& x)
	effects  - creates an array that is a copy of x.

    Array(T x, int count)
	requires - count >= 0
	effects  - creates an array with count copies of x.
    */

// \subsection{Destructor Specifications}

    /*
    ~Array()
	effects  - releases all storage for the array.
    */

// \subsection{Operation Specifications}

    /*
    Array& operator=(Array const& x)
	effects  - copies the contents of x into *this.

    T& operator[](int index) const
	requires - index >= 0 and index < size().
	effects  - returns a reference to the indexth element in *this.
		   Therefore, you can say things like --
			Array a;
			...
			a[i] = a[i+1];

    T& slot(int index) const
	requires - index >= 0 and index < size().
	effects  - returns a reference to the indexth element in *this.
		   This operation is identical to operator [].  It is
		   just more convenient to use with an Array*.

			Array* a;
			...
			a->slot(i) = a->slot(i+1);

    int  size() const
	effects	 - returns the number of elements in *this.

    T& high() const
	effects  - returns a reference to the last element in *this.

    void append(T v)
	modifies - *this
	effects  - grows array by one by adding v to the high end.

    void append(T v, int n)
	requires - n >= 0
	modifies - *this
	effects  - grows array by n by adding n copies of v to the high end.

    void concat(T const* x, int n)
	requires - n >= 0, x points to at least n Ts.
	modifies - *this
	effects	 - grows array by n by adding the n elements pointed to by x.

    void concat(Array const& x)
	modifies - *this
	effects  - append the contents of x to *this.

    T remove()
	requires - array is not empty
	modifies - *this
	effects  - removes last element and return a copy of it.

    void remove(int num)
	requires - num >= 0 and array has at least num elements
	modifies - *this
	effects  - removes the last num elements.

    void clear()
	modifies - *this
	effects  - removes all elements from *this.

    void reclaim()
	effects  - reclaim unused storage.

    T* as_pointer() const
	requires - returned value is not used across changes in array size
		   or calls to reclaim.
	effects  - returns a pointer to the first element in the array.
		   The returned pointer is useful for interacting with
		   code that manipulates builtin arrays of T.

    void predict(int new_alloc);
         effects - Does not change the abstract state. If the allocated 
                   storage is smaller than new_alloc elements, enlarges the
                   storage to new_alloc elements.
 
    void _enlarge_by(int n);
	requires - n >= 0
	effects  - appends "n" UNITIALIZATED entries to the array.
		   This is an unsafe operation that is mostly useful
		   when reading the contents of an array over the net.
		   Use it carefully.

    */

#ifndef _Array_h
#define _Array_h 1

#include "th_assert.h"

template <class T> class Array {
  public:
								
    /* Constructors */						
    Array();			/* Empty array */	
    Array(int predict);		/* Empty array with size predict */
    Array(T const*, int);	/* Initialized with C array */	  
    Array(Array const&);	/* Initialized with another Array */
    Array(T, int);		/* Fill with n copies of T */ 
									    
    /* Destructor */							    
    ~Array();							    
									    
    /* Assignment operator */						    
    Array& operator=(Array const&);				    
									    
    /* Array indexing */						    
    T& operator[](int index) const;				   
    T& slot(int index) const;					   
									   
    /* Other Array operations */				
    int  size() const;			/* Return size; */	
    T& high() const;		/* Return last T */	
    T* as_pointer() const;	/* Return as pointer to base */	
    void append(T v);		/* append an T */		
    void append(T, int n);	/* Append n copies of T */ 
    void concat(T const*, int);	/* Concatenate C array */	 
    void concat(Array const&);	/* Concatenate another Array */	     
    T remove();			/* Remove and return last T */
    void remove(int num);		/* Remove last num Ts */	    
    void clear();			/* Remove all Ts */	    
    void predict(int new_alloc);        /* Increase allocation */	    
									    
									    
    /* Storage stuff */							    
    void reclaim();			/* Reclaim all unused space */	    
    void _enlarge_by(int n);		/* Enlarge array by n */	    
    void enlarge_to(int s);		/* Enlarge to s if necessary */	    
  private:								    
    T*	store_;			/* Actual storage */		    
    int		alloc_;			/* Size of allocated storage */	    
    int		size_;			/* Size of used storage */	    
									    
    /* Storage enlargers */						    
    void enlarge_allocation_to(int s);	/* Enlarge to s */		    
};									    

template <class T>
inline Array<T>::Array() {						      
    alloc_ = 0;								      
    size_  = 0;								      
    store_ = 0;								      
}
									      
template <class T>								      
inline int Array<T>::size() const {					      
    return size_;							      
}			
						      
template <class T>								      
inline T& Array<T>::operator[](int index) const {		      
    th_assert((index >= 0) && (index < size_), "array index out of bounds");  
    return store_[index];						      
}									      

template <class T>								      
inline T& Array<T>::slot(int index) const {			      
    th_assert((index >= 0) && (index < size_), "array index out of bounds");  
    return store_[index];						      
}									      
			
template <class T>					      
inline T& Array<T>::high() const {				      
    th_assert(size_ > 0, "array index out of bounds");			      
    return store_[size_-1];						      
}									      
			
template <class T>				      
inline T* Array<T>::as_pointer() const {				      
    return store_;							      
}									      
			
template <class T>				      
inline void Array<T>::append(T v) {				      
    if (size_ >= alloc_)						      
	enlarge_allocation_to(size_+1);					      
    store_[size_++] = v;						      
}									      
			
template <class T>				      
inline T Array<T>::remove() {					      
    if (size_ > 0) size_--;						      
    return store_[size_];						      
}									      
			
template <class T>				      
inline void Array<T>::remove(int num) {				      
    th_assert((num >= 0) && (num <= size_), "invalid array remove count");    
    size_ -= num;							      
}									      
			
template <class T>				      
inline void Array<T>::clear() {					      
    size_ = 0;								      
}									      
			
template <class T>				      
inline void Array<T>::_enlarge_by(int n) {				      
    th_assert(n >= 0, "negative count supplied to array operation");	      
    int newsize = size_ + n;                                                  
    if (newsize > alloc_)						      
	enlarge_allocation_to(newsize);					      
    size_ = newsize;                                                          
}									      
							  

template <class T>								     
Array<T>::Array(int predict) {					    
    th_assert(predict >= 0, "negative count supplied to array operation");
    alloc_ = 0;								  
    size_ = 0;								  
    store_ = 0;								  
    enlarge_to(predict);						  
    size_ = 0;								  
}									  

template <class T>								
Array<T>::~Array() {						    
    if (alloc_ > 0) delete [] store_;					
}

template <class T>
Array<T>::Array(T const* src, int s) {
    th_assert(s >= 0, "negative count supplied to array operation");
    alloc_ = 0;								
    size_  = 0;								
    store_ = 0;								
    enlarge_to(s);							
    for (int i = 0; i < s; i++)						
	store_[i] = src[i];						
}									

template <class T>							
Array<T>::Array(Array const& d) {				    
    alloc_ = 0;							
    size_  = 0;							
    store_ = 0;							
    enlarge_to(d.size_);					
    for (int i = 0; i < size_; i++)				
	store_[i] = d.store_[i];				
}								

template <class T>		
Array<T>::Array(T element, int num) {	
    th_assert(num >= 0, "negative count supplied to array operation");	      
    alloc_ = 0;								      
    size_ = 0;								      
    store_ = 0;								      
    enlarge_to(num);							      
    for (int i = 0; i < num; i++)					      
	store_[i] = element;						      
}									      

template <class T>							
Array<T>& Array<T>::operator=(Array const& d) {			      
    size_ = 0;								      
    enlarge_to(d.size_);						      
    for (int i = 0; i < size_; i++)					      
	store_[i] = d.store_[i];					      
    return (*this);							      
}									      

									      
template <class T>
void Array<T>::append(T element, int n) {			      
    th_assert(n >= 0, "negative count supplied to array operation");	      
    int oldsize = size_;	
    enlarge_to(size_ + n);		
    for (int i = 0; i < n; i++)		
	store_[i + oldsize] = element;	
}					
		
template <class T>			
void Array<T>::concat(Array const& d) {	
    int oldsize = size_;		
    enlarge_to(size_ + d.size_);	
    for (int i = 0; i < d.size_; i++)	
	store_[i+oldsize] = d.store_[i];
}					
			
template <class T>		
void Array<T>::concat(T const* src, int s) {
    th_assert(s >= 0, "negative count supplied to array operation");
    int oldsize = size_;						
    enlarge_to(s + size_);						
    for (int i = 0; i < s; i++)						
	store_[i+oldsize] = src[i];					
}									
			
template <class T>					
void Array<T>::predict(int new_alloc) {				
    if (new_alloc > alloc_)					
        enlarge_allocation_to(new_alloc);			
}								
 			
template <class T>					
void Array<T>::enlarge_to(int newsize) {				
    if (newsize > alloc_)					
	enlarge_allocation_to(newsize);				
   size_ = newsize;						
}								
			
template <class T>					
void Array<T>::enlarge_allocation_to(int newsize) {		
    int newalloc = alloc_ * 2;					
    if (newsize > newalloc) newalloc = newsize;			
								
    T* oldstore = store_;					
    store_ = new T[newalloc];					
								
    for (int i = 0; i < size_; i++)				
	store_[i] = oldstore[i];				
								
    if (alloc_ > 0) delete [] oldstore;			
    alloc_ = newalloc;					
}							
			
template <class T>				
void Array<T>::reclaim() {					
    if (alloc_ > size_) {				
	/* Some free entries that can be reclaimed */	
	if (size_ > 0) {				
	    /* Array not empty - create new store */	
	    T* newstore = new T[size_];			
	    for (int i = 0; i < size_; i++)		
		newstore[i] = store_[i];		
	    delete [] store_;				
	    alloc_ = size_;				
	    store_ = newstore;				
	}						
	else {						
	    /* Array empty - delete old store */	
	    if (alloc_ > 0) {				
		delete [] store_;			
		alloc_ = 0;				
	    }						
	}						
    }							
}	

#endif // _Array_h

