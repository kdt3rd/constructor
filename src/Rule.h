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


////////////////////////////////////////


class Rule
{
public:
	Rule( std::string tag, std::string desc );
	virtual ~Rule( void );

	inline void setDependencyFile( const std::string &dname );
	inline const std::string &dependencyFile( void ) const;

	inline void setJobPool( const std::string &jname );
	inline const std::string &jobPool( void ) const;

	inline void setOutputRestat( bool r );
	inline bool outputRestat( void ) const;

	void command( const std::string &c );
	void command( const std::vector<std::string> &c );
	void addCommand( const std::vector<std::string> &c );

	inline const std::vector<std::string> &rawCommand( void ) const;

	std::string command( void ) const;
	template <typename VF>
	inline std::string command( VF &&f ) const
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

	inline const std::string &description( void ) const;

private:
	std::string myTag;
	std::string myDesc;
	std::string myDepFile;
	std::string myJobPool;
	std::vector<std::string> myCommand;
	bool myOutputRestat = false;
};


////////////////////////////////////////


inline void
Rule::setDependencyFile( const std::string &dname )
{
	myDepFile = dname;
}
inline const std::string &
Rule::dependencyFile( void ) const
{
	return myDepFile;
}


////////////////////////////////////////


inline void
Rule::setJobPool( const std::string &jname )
{
	myJobPool = jname;
}
inline const std::string &
Rule::jobPool( void ) const
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
Rule::outputRestat( void ) const
{
	return myOutputRestat;
}


////////////////////////////////////////


inline const std::vector<std::string> &
Rule::rawCommand( void ) const 
{
	return myCommand;
}


////////////////////////////////////////


inline const std::string &
Rule::description( void ) const 
{
	return myDesc;
}

