#include <QTest>
#include <QtQuickTest/quicktest.h>
#include <QMetaObject>
#include <QMetaMethod>
#include <QPointer>
#include <QCommandLineParser>
#include "testrunner.h"
#include "automator.h"

static TestRunner *m_defaultInstance = 0;

TestRunner::TestRunner()
{
    if (m_defaultInstance == 0) {
        m_defaultInstance = this;
    }

    m_engineHook = 0;

    m_gallery = [](const QStringList& arguments) {
        Q_UNUSED(arguments);
        qWarning() << "TestRunner: Gallery function is not set";
        return false;
    };
}

TestRunner::~TestRunner()
{
    QObject* object;
    QVariant item;

    foreach (item, m_testObjects) {
        object = item.value<QObject*>();
        if (object) {
            delete object;
        }
    }
    m_testObjects.clear();

    if (m_defaultInstance == this) {
        m_defaultInstance = 0;
    }
}

void TestRunner::add(QObject *object)
{
    QVariant value = QVariant::fromValue<QObject*>(object);
    m_testObjects.append(value);
}

void TestRunner::add(const QString &path)
{
    add(QVariant(path));
}

bool TestRunner::exec(QStringList arguments)
{

    QCommandLineParser parser;

    QCommandLineOption galleryOption (QStringList() << "gallery");
    parser.addOption(galleryOption);
    parser.parse(arguments);

    if (parser.isSet(galleryOption)) {
        return runGallery(parser.positionalArguments());
    }

    QObject *object;
    QVariant item;
    bool error = false;
    m_arguments = arguments;

    foreach (item,m_testObjects) {
        object = item.value<QObject*>();
        if (object) {
            error |= run(object, arguments);
        } else if (item.type() == (int) QMetaType::QString) {
            error |= run(item.toString(), arguments);
        }
    }

    return error;
}

void TestRunner::addImportPath(const QString &path)
{
    m_importPaths << path;
}

int TestRunner::count() const
{
    return m_testObjects.size();
}

void TestRunner::add(QVariant value)
{
    m_testObjects.append(value);
}

bool TestRunner::run(QObject *object, const QStringList& arguments)
{
    const QMetaObject *meta = object->metaObject();
    QStringList args = arguments;
    QString executable = args.takeAt(0);
    bool execute; // Should it execute this test object?
    QStringList params; // params for qTest
    params << executable;

    QStringList tmp;

    // Always add "-*" to params.
    foreach (QString arg, args) {
        if (arg.indexOf("-") == 0) {
            params << arg;
        } else {
            tmp << arg;
        }
    }

    args = tmp;

    execute = args.size() > 0 ? false : true; // If no argument passed , it should always execute a test object

    foreach (QString arg, args) {

        if (arg == meta->className()) {
            execute = true;
            break;
        }

        for (int i = 0 ; i < meta->methodCount();i++){
            if (meta->method(i).name() == arg) {
                params << arg;
                execute = true;
            }
        }
    }

    if (!execute) {
        return false;
    }

    return QTest::qExec(object,params);
}

bool TestRunner::run(QString path, const QStringList &arguments)
{
    QStringList args(arguments);
    QString executable = args.takeAt(0);
    QStringList testcases = arguments.filter(QRegExp("::"));

    QStringList nonOptionArgs; // Filter all "-" parameter
    foreach (QString arg, args) {
        if (arg.indexOf("-") != 0) {
            nonOptionArgs << arg;
        }
    }

    if (nonOptionArgs.size() !=0 && testcases.size() == 0) {
        // If you non-option args, but not a quick test case. Return
        return false;
    }

    QStringList paths;
    paths << path;
    paths << m_importPaths;

    char **s = (char**) malloc(sizeof(char*) * (10 + args.size() + paths.size() * 2));
    int idx = 0;
    s[idx++] = executable.toUtf8().data();

    foreach (QString p , paths) {
        s[idx++] = strdup("-import");
        s[idx++] = strdup(p.toUtf8().data());
    }

    foreach( QString arg,args) {
        s[idx++] = strdup(arg.toUtf8().data());
    }
    s[idx++] = 0;

    const char *name = "QuickTests";
    const char *source = strdup(path.toUtf8().data());

    bool error = quick_test_main( idx-1, s, name, source);
    free(s);

    return error;
}

bool TestRunner::runGallery(const QStringList &arguments)
{
    return m_gallery(arguments);
}

QVariantMap TestRunner::config() const
{
    return m_config;
}

void TestRunner::setConfig(const QVariantMap &config)
{
    m_config = config;
}

void TestRunner::execEngineHook(QQmlEngine *engine)
{
    if (m_engineHook != 0) {
        m_engineHook(engine);
    }
}

void TestRunner::setEngineHook(TestRunner::Callback func)
{
    m_engineHook = func;
}

QStringList TestRunner::arguments() const
{
    return m_arguments;
}

TestRunner *TestRunner::defautInstance()
{
    return m_defaultInstance;
}

void TestRunner::setGallery(std::function<bool(const QStringList &)> function)
{
    m_gallery = function;
}

std::function<bool (const QStringList &)> TestRunner::gallery() const
{
    return m_gallery;
}

