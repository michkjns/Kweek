#pragma once
#include "template.h"
#include <stdarg.h>
#include <list>
#include <string>

namespace Tmpl8 
{

	class Surface;
	class Log
	{
	public:
		static Log* Get();
		Log();
		~Log();
		int m_maxMessages;
		void Print( std::string msg, ... );
		void Draw( Surface* a_Surface );
	private:
		static Log* instance;
		std::list<std::string> messages;
		Timer m_timer;
	};

#define LOG(...) Log::Get()->Print(__VA_ARGS__);

};