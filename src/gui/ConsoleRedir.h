#ifndef CONSOLEREDIR_H_
#define CONSOLEREDIR_H_

#include <iostream>
#include <streambuf>
#include <string>

#include <QtGui/QTextEdit>
#include <QtCore/QTextStream>
#include <QtGui/QScrollBar>
class ConsoleRedir : public std::basic_streambuf<char>
{
public:
	ConsoleRedir(std::ostream &stream, QTextEdit* text_edit) : conStream(stream)
	{
		console = text_edit;
		oldBufer = conStream.rdbuf();
		conStream.rdbuf(this);
		stringBuffer = "";
		streamT = new QTextStream(&stringBuffer);
		
		console->setHtml("<body bgcolor=\"#000000\">");
		sb = console->verticalScrollBar();
		
		//console->setTextBackgroundColor(Qt::black);
	}
	~ConsoleRedir()
	{
		if (!stringBuffer.isEmpty()) console->append(stringBuffer);
		conStream.rdbuf(oldBufer);
	}
	void FlushReminder()
	{
		if(stringBuffer.size() > 0)
		{
			insertColored(0);
		}
	}
	
protected:
	virtual int_type overflow(int_type v)
	{
		
		if (v == '\n' || v == '\r') insertColored(0);
		else *streamT << v;

		return v;
	}
	
	virtual std::streamsize xsputn(const char *p, std::streamsize n)
	{
		QString temp = p;
		temp.chop(temp.size() - n);
		
		*streamT << temp;
		
		insertColored();
		
		return n;
	}
	
private:
	void insertColored(int off = 1)
	{
		QStringList list = stringBuffer.split('\n');
		if(list.size() > off)
		{
			for(int i = 0; i < list.size()-off; i++)
			{
				if(list[i].startsWith("INFO:")) console->setTextColor(qRgb(0, 200, 0)); // green
				else if(list[i].startsWith("WARNING:")) console->setTextColor(qRgb(0, 200, 97)); // orange
				else if(list[i].startsWith("ERROR:")) console->setTextColor(qRgb(200, 0, 0)); // red
				else  console->setTextColor(qRgb(200, 200, 200)); // light-gray
				console->append(list[i]);
				sb->setValue(sb->maximum());
			}
			stringBuffer.clear();
			if(off > 0) *streamT << list[list.size()-1];
		}
	}
	std::ostream &conStream;
	std::streambuf *oldBufer;
	QString stringBuffer;
	QTextStream *streamT;
	QTextEdit *console;
	QScrollBar *sb;
};

#endif
