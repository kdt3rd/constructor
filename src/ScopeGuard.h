//
// Copyright (c) 2014 Kimball Thurston
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
// CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
// OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//

#pragma once

#include <functional>


////////////////////////////////////////


class ScopeGuard
{
public:
	inline ScopeGuard( void ) {}
	inline ScopeGuard( const std::function<void()> &f ) : myF( f ) {}
	inline ScopeGuard( std::function<void()> &&f ) : myF( std::move( f ) ) {}
	template <typename Lambda>
	inline ScopeGuard( Lambda &&f ) : myF( std::forward( f ) ) {}
	inline ScopeGuard( ScopeGuard &&o ) noexcept { myF.swap( o.myF ); }
	
	inline ~ScopeGuard( void )
	{
		if ( myF )
			myF();
	}

	inline ScopeGuard &operator=( ScopeGuard &&o ) noexcept
	{
		if ( myF )
			myF();
		myF = std::move( o.myF );
		o.myF = nullptr;
		return *this;
	}

	inline void release( void ) noexcept { myF = nullptr; }
private:
	ScopeGuard( const ScopeGuard & ) = delete;
	ScopeGuard &operator=( const ScopeGuard & ) = delete;

	std::function<void()> myF;
};

#define CONCATENATE_DIRECT( s1, s2 ) s1##s2
#define CONCATENATE( s1, s2 ) CONCATENATE_DIRECT( s1, s2 )
#define ANONYMOUS_VAR( str ) CONCATENATE( str, __LINE__ )

#define ON_EXIT [[gnu::unused]] auto ANONYMOUS_VAR( exitGuard ) = [&]() 
