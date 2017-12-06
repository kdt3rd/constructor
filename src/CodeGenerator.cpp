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

#include "CodeGenerator.h"

#include "Debug.h"
#include <fstream>
#include <sstream>
#include <map>
#include "Directory.h"
#include "StrUtil.h"


////////////////////////////////////////


CodeGenerator::CodeGenerator( std::string name )
		: CompileSet( std::move( name ) )
{
}


////////////////////////////////////////


CodeGenerator::~CodeGenerator( void )
{
}


////////////////////////////////////////


void
CodeGenerator::setItemInfo( const std::vector<std::string> &itemPrefix,
							const std::vector<std::string> &itemSuffix,
							const std::string &itemIndent,
							bool doCommas )
{
	myItemPrefix = itemPrefix;
	myItemSuffix = itemSuffix;
	myItemIndent = itemIndent;
	myDoCommas = doCommas;
}


////////////////////////////////////////


void
CodeGenerator::setFileInfo( const std::vector<std::string> &filePrefix,
							const std::vector<std::string> &fileSuffix )
{
	myFilePrefix = filePrefix;
	myFileSuffix = fileSuffix;
}


////////////////////////////////////////


std::shared_ptr<BuildItem>
CodeGenerator::transform( TransformSet &xform ) const
{
	std::shared_ptr<BuildItem> ret = xform.getTransform( getID() );
	if ( ret )
		return ret;

	DEBUG( "transform CodeGenerator " << getName() );
	auto outd = getDir()->reroot( xform.getArtifactDir() );
	ret = std::make_shared<BuildItem>( getName(), outd );

	VariableSet buildvars;
	extractVariables( buildvars );
	ret->setVariables( std::move( buildvars ) );

	ret->setTool( xform.getTool( "codegen_binary_cstring" ) );
	ret->setOutputDir( outd );
	ret->setOutputs( { getName() } );
	ret->setUseName( false );

	std::vector<std::string> codegenVar;
	if ( myDoCommas )
		codegenVar.push_back( "-comma" );
	auto tmpd = std::make_shared<Directory>( *outd );
	tmpd->cd( ".codegen" );
	processEntry( "file_prefix", tmpd, myFilePrefix, ret, codegenVar );
	processEntry( "file_suffix", tmpd, myFileSuffix, ret, codegenVar );
	processEntry( "item_prefix", tmpd, myItemPrefix, ret, codegenVar );
	processEntry( "item_suffix", tmpd, myItemSuffix, ret, codegenVar );
	if ( ! myItemIndent.empty() )
		processEntry( "item_indent", tmpd, { myItemIndent }, ret, codegenVar );

	ret->setVariable( "codegen_info", codegenVar );
	for ( const ItemPtr &i: myItems )
	{
		// huh, we can't call transform on these because then
		// cpp files we're going to filter will be transformed to
		// the output blob
		auto inp = std::make_shared<BuildItem>( i->getName(), i->getDir() );
		inp->setUseName( false );
		inp->setOutputDir( getDir() );
		inp->setOutputs( { i->getName() } );
		ret->addDependency( DependencyType::EXPLICIT, inp );
	}

	xform.recordTransform( getID(), ret );
	return ret;
}


////////////////////////////////////////


void
CodeGenerator::emitCode( const std::string &outfn,
						 const std::vector<std::string> &inputs,
						 const std::string &filePrefix,
						 const std::string &fileSuffix,
						 const std::string &itemPrefix,
						 const std::string &itemSuffix,
						 const std::string &itemIndent,
						 bool doCommas )
{
	std::vector<std::string> outlines;

	if ( ! filePrefix.empty() )
	{
		std::ifstream prefixf( filePrefix );
		std::string curLine;
		while ( std::getline( prefixf, curLine ) )
			outlines.emplace_back( std::move( curLine ) );
	}

	std::vector<std::string> itemPrefixL, itemSuffixL, itemIndentL;
	if ( ! itemPrefix.empty() )
	{
		std::ifstream prefixf( itemPrefix );
		std::string curLine;
		while ( std::getline( prefixf, curLine ) )
			itemPrefixL.emplace_back( std::move( curLine ) );
	}
	if ( ! itemSuffix.empty() )
	{
		std::ifstream suffixf( itemSuffix );
		std::string curLine;
		while ( std::getline( suffixf, curLine ) )
			itemSuffixL.emplace_back( std::move( curLine ) );
	}
	if ( ! itemIndent.empty() )
	{
		std::ifstream indentf( itemIndent );
		std::string curLine;
		while ( std::getline( indentf, curLine ) )
			itemIndentL.emplace_back( std::move( curLine ) );
	}

	for ( size_t i = 0; i != inputs.size(); ++i )
	{
		const std::string &curInp = inputs[i];
		std::ifstream inpf( curInp, std::ifstream::binary );
		if ( inpf )
		{
			inpf.seekg( 0, inpf.end );
			ssize_t nbytes = inpf.tellg();
			inpf.seekg( 0, inpf.beg );
			std::string tmpmem;
			tmpmem.resize( static_cast<size_t>( nbytes ), '\0' );
			char *begin = &*tmpmem.begin();
			inpf.read( begin, nbytes );
			if ( inpf.gcount() != nbytes )
				throw std::runtime_error( "Unable to read all the data from '" + curInp + "'" );

			inpf.close();

			std::map<std::string, std::string> vars;
			{
				Directory tmpInp( curInp );
				vars["item_name"] = tmpInp.cur();
			}
			{
				std::stringstream tmpBuf;
				tmpBuf << nbytes;
				vars["item_file_size"] = tmpBuf.str();
			}

			for ( const std::string &iPref: itemPrefixL )
			{
				std::string tmp = iPref;
				String::substituteVariables( tmp, false, vars );
				outlines.emplace_back( std::move( tmp ) );
			}

			bool atLineBeg = true;
			std::string curLine;
			static const char hexStr[] = "0123456789ABCDEF";
			for ( size_t outb = 0; outb < static_cast<size_t>( nbytes ); ++outb )
			{
				if ( atLineBeg )
				{
					for ( const std::string &ind: itemIndentL )
						curLine.append( ind );
					curLine.push_back( '"' );
					atLineBeg = false;
				}

				curLine.push_back( '\\' );
				curLine.push_back( 'x' );
				int ux = tmpmem[outb];
				int lx = ux % 16;
				ux = ux / 16;
				if ( ux > 15 || lx > 15 )
					throw std::runtime_error( "Unexpected hex value" );
				curLine.push_back( hexStr[ux] );
				curLine.push_back( hexStr[lx] );

				if ( ( outb + 1 ) % 20 == 0 )
				{
					curLine.push_back( '"' );
					outlines.emplace_back( std::move( curLine ) );
					atLineBeg = true;
				}
			}
			if ( nbytes == 0 )
				outlines.push_back( "\"\"" );
			else if ( ! atLineBeg )
			{
				curLine.push_back( '"' );
				outlines.emplace_back( std::move( curLine ) );
			}

			for ( const std::string &iSuff: itemSuffixL )
			{
				std::string tmp = iSuff;
				String::substituteVariables( tmp, false, vars );
				outlines.emplace_back( std::move( tmp ) );
			}
		}
		else
			throw std::runtime_error( "Unable to open '" + curInp + "' for read" );
		
		if ( doCommas && ( i + 1 ) < inputs.size() )
			outlines.back().push_back( ',' );
	}

	if ( ! fileSuffix.empty() )
	{
		std::ifstream suffixf( fileSuffix );
		std::string curLine;
		while ( std::getline( suffixf, curLine ) )
			outlines.emplace_back( std::move( curLine ) );
	}

	Directory tmpd( outfn );
	std::string fn = tmpd.cur();
	tmpd.cdUp();
	tmpd.updateIfDifferent( fn, outlines );
}


////////////////////////////////////////


void
CodeGenerator::processEntry( const std::string &tag,
							 const std::shared_ptr<Directory> &tmpd,
							 const std::vector<std::string> &list,
							 const std::shared_ptr<BuildItem> &ret,
							 std::vector<std::string> &varlist ) const
{
	if ( list.empty() )
		return;

	std::string tmpname = tag + "_" + getName();
	tmpd->updateIfDifferent( tmpname, list );
	auto inp = std::make_shared<BuildItem>( tmpname, tmpd );
	inp->setUseName( false );
	inp->setOutputDir( tmpd );
	inp->setOutputs( { tmpname } );
	ret->addDependency( DependencyType::IMPLICIT, inp );
	varlist.push_back( "-" + tag );
	varlist.push_back( tmpd->makefilename( tmpname ) );
}


////////////////////////////////////////


