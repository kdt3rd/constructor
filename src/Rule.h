//
// Copyright (c) 2013 Kimball Thurston
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

#include <string>
#include <vector>


////////////////////////////////////////


class Rule
{
public:
	Rule( std::string tag, std::string desc );
	Rule( Rule &&r );
	~Rule( void );

	Rule &operator=( Rule &&r );

	inline const std::string &getName( void ) const;
	inline const std::string &getDescription( void ) const;

	inline void setDependencyFile( const std::string &dname );
	inline const std::string &getDependencyFile( void ) const;

	inline void setDependencyStyle( const std::string &dname );
	inline const std::string &getDependencyStyle( void ) const;

	inline void setJobPool( const std::string &jname );
	inline const std::string &getJobPool( void ) const;

	inline void setOutputRestat( bool r );
	inline bool isOutputRestat( void ) const;

	void setCommand( const std::string &c );
	void setCommand( const std::vector<std::string> &c );
	void setCommand( std::vector<std::string> &&c );
	void addToCommand( const std::vector<std::string> &c );

	inline const std::vector<std::string> &getRawCommand( void ) const;

	std::string getCommand( void ) const;

	template <typename VF>
	inline std::string getCommand( VF &&f ) const
	{
		std::string ret;
		for ( const std::string &i: myCommand )
		{
			if ( i.empty() )
				continue;
			if ( ! ret.empty() )
				ret.push_back( ' ' );

			ret.append( f( i ) );
		}
		return std::move( ret );
	}


private:
	std::string myTag;
	std::string myDesc;
	std::string myDepFile;
	std::string myDepStyle;
	std::string myJobPool;
	std::vector<std::string> myCommand;
	bool myOutputRestat = false;
};


////////////////////////////////////////


inline const std::string &
Rule::getName( void ) const 
{
	return myTag;
}
inline const std::string &
Rule::getDescription( void ) const 
{
	return myDesc;
}


////////////////////////////////////////


inline void
Rule::setDependencyFile( const std::string &dname )
{
	myDepFile = dname;
}
inline const std::string &
Rule::getDependencyFile( void ) const
{
	return myDepFile;
}


////////////////////////////////////////


inline void
Rule::setDependencyStyle( const std::string &dname )
{
	myDepStyle = dname;
}
inline const std::string &
Rule::getDependencyStyle( void ) const
{
	return myDepStyle;
}


////////////////////////////////////////


inline void
Rule::setJobPool( const std::string &jname )
{
	myJobPool = jname;
}
inline const std::string &
Rule::getJobPool( void ) const
{
	return myJobPool;
}


////////////////////////////////////////


inline void
Rule::setOutputRestat( bool r )
{
	myOutputRestat = r;
}
inline bool
Rule::isOutputRestat( void ) const
{
	return myOutputRestat;
}


////////////////////////////////////////


inline const std::vector<std::string> &
Rule::getRawCommand( void ) const 
{
	return myCommand;
}

