#include <QApplication>
#include <QPushButton>

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    QPushButton btn("Hello, world!");
    QObject::connect(&btn, &QPushButton::clicked, &app, &QCoreApplication::quit);
    btn.show();

    return app.exec();
}
