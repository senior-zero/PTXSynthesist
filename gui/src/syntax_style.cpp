#include <syntax_style.h>

#include <QDebug>
#include <QXmlStreamReader>
#include <QFile>

SyntaxStyle::SyntaxStyle(QObject* parent) :
        QObject(parent),
        m_name(),
        m_data(),
        m_loaded(false)
{

}

bool SyntaxStyle::load(QString fl)
{
    QXmlStreamReader reader(fl);

    while (!reader.atEnd() && !reader.hasError())
    {
        auto token = reader.readNext();

        if(token == QXmlStreamReader::StartElement)
        {
            if (reader.name() == QString("style-scheme"))
            {
                if (reader.attributes().hasAttribute("name"))
                {
                    m_name = reader.attributes().value("name").toString();
                }
            }
            else if (reader.name() == QString("style"))
            {
                auto attributes = reader.attributes();

                auto name = attributes.value("name");

                QTextCharFormat format;

                if (attributes.hasAttribute("background"))
                {
                    format.setBackground(QColor(attributes.value("background").toString()));
                }

                if (attributes.hasAttribute("foreground"))
                {
                    format.setForeground(QColor(attributes.value("foreground").toString()));
                }

                if (attributes.hasAttribute("bold") &&
                    attributes.value("bold") == QString("true"))
                {
                    format.setFontWeight(QFont::Weight::Bold);
                }

                if (attributes.hasAttribute("italic") &&
                    attributes.value("italic") == QString("true"))
                {
                    format.setFontItalic(true);
                }

                if (attributes.hasAttribute("underlineStyle"))
                {
                    auto underline = attributes.value("underlineStyle");

                    auto s = QTextCharFormat::UnderlineStyle::NoUnderline;

                    if (underline == QString("SingleUnderline"))
                    {
                        s = QTextCharFormat::UnderlineStyle::SingleUnderline;
                    }
                    else if (underline == QString("DashUnderline"))
                    {
                        s = QTextCharFormat::UnderlineStyle::DashUnderline;
                    }
                    else if (underline == QString("DotLine"))
                    {
                        s = QTextCharFormat::UnderlineStyle::DotLine;
                    }
                    else if (underline == QString("DashDotLine"))
                    {
                        s = QTextCharFormat::DashDotLine;
                    }
                    else if (underline == QString("DashDotDotLine"))
                    {
                        s = QTextCharFormat::DashDotDotLine;
                    }
                    else if (underline == QString("WaveUnderline"))
                    {
                        s = QTextCharFormat::WaveUnderline;
                    }
                    else if (underline == QString("SpellCheckUnderline"))
                    {
                        s = QTextCharFormat::SpellCheckUnderline;
                    }
                    else
                    {
                        qDebug() << "Unknown underline value " << underline;
                    }

                    format.setUnderlineStyle(s);
                }

                m_data[name.toString()] = format;
            }
        }
    }

    m_loaded = !reader.hasError();

    return m_loaded;
}

QString SyntaxStyle::name() const
{
    return m_name;
}

QTextCharFormat SyntaxStyle::getFormat(QString name) const
{
    auto result = m_data.find(name);

    if (result == m_data.end())
    {
        return QTextCharFormat();
    }

    return result.value();
}

bool SyntaxStyle::isLoaded() const
{
    return m_loaded;
}

SyntaxStyle* SyntaxStyle::defaultStyle()
{
    static SyntaxStyle style;

    if (!style.isLoaded())
    {
        QFile fl(":/style/default.xml");

        if (!fl.open(QIODevice::ReadOnly))
        {
            return &style;
        }

        if (!style.load(fl.readAll()))
        {
            qDebug() << "Can't load default style.";
        }
    }

    return &style;
}

