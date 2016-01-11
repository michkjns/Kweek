#include "log.h"
#include "surface.h"
#include <memory>
#include "input.h"
#include "template.h"
using namespace Tmpl8;

Log* Log::instance = nullptr;


Log* Log::Get()
{
	if(!instance)
	{
		instance = new Log();
	}
	return instance;
}

void Log::Print( std::string msg, ... )
{
	while( messages.size() > 5 ) messages.pop_front();

	 int final_n, n = msg.size() * 2; 
    std::string str;
    std::unique_ptr<char[]> formatted;
    va_list ap;
    while(1) {
        formatted.reset(new char[n]); 
        strcpy(&formatted[0], msg.c_str());
        va_start(ap, msg);
        final_n = vsnprintf(&formatted[0], n, msg.c_str(), ap);
        va_end(ap);
        if (final_n < 0 || final_n >= n)
            n += abs(final_n - n + 1);
        else
            break;
    }
	printf( "%s\n", formatted.get() );
	messages.push_back( formatted.get() );
	m_timer.reset();
}

void Log::Draw( Surface* a_Surface )
{
	int c = 0;
	for( auto msg = messages.begin(); msg != messages.end(); ++msg)
	{
		a_Surface->Print( (char*)(*msg).c_str(), 10, 100 + c * 10, 0xffffff);
		c++;
	}
	if( m_timer.seconds() > 10 )
	{
		m_timer.reset();
		if( messages.size() > 1 ) messages.pop_front();
	}
}

Log::Log()
{
	m_maxMessages = 5;
}

Log::~Log()
{
	instance = nullptr;
}