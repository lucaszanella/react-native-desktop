
/**
 * Copyright (c) 2017-present, Status Research and Development GmbH.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "webengineqtexecutor.h"
#include "bridge.h"

#include <QJsonDocument>
#include <QSharedPointer>

#include <QStandardPaths>

#include <QDir>
#include <QEventLoop>
#include <QWebEnginePage>
#include <QWebEnginePage>
#include <QWebEngineProfile>
#include <QWebEngineScript>
#include <QWebEngineScriptCollection>
#include <QWebEngineSettings>
#include <QWebEngineView>

#include <QNetworkProxy>

#include <QFile>
#include <QWebChannel>

class WebEngineQtExecutorPrivate : public QObject {
public:
    WebEngineQtExecutorPrivate(WebEngineQtExecutor* e) : QObject(e), q_ptr(e) {}

    WebEngineQtExecutor* q_ptr = nullptr;
    QWebEnginePage* myPage = nullptr;

    void init();
};

void WebEngineQtExecutorPrivate::init() {

    myPage = new QWebEnginePage();

    //        QEventLoop* eventLoop = new QEventLoop();

    //        connect(myPage, &QWebEnginePage::loadFinished, this, [=](bool finished) {
    //            qDebug() << "!!! load finished with result: " << finished;
    //            eventLoop->exit();
    //        });

    //        myPage->setHtml(QString("<html></html>"));

    //        eventLoop->exec();
}

WebEngineQtExecutor::WebEngineQtExecutor(QObject* parent)
    : IExecutor(parent), d_ptr(new WebEngineQtExecutorPrivate(this)) {
    qRegisterMetaType<IExecutor::ExecuteCallback>();
    qRegisterMetaType<QNetworkProxy>("QNetworkProxy");
    qRegisterMetaType<QAbstractSocket::SocketError>();
}

void WebEngineQtExecutor::initJSconstraints() {
    d_ptr->init();
}

WebEngineQtExecutor::~WebEngineQtExecutor() {
    resetConnection();
}

void WebEngineQtExecutor::injectJson(const QString& name, const QVariant& data) {
    Q_D(WebEngineQtExecutor);
    QJsonDocument doc = QJsonDocument::fromVariant(data);
    QString code = name.toLocal8Bit() + "=" + doc.toJson(QJsonDocument::Compact) + ";";

    qDebug() << "injectJson: " << code;

    d_ptr->myPage->runJavaScript(code, 0, [](const QVariant& v) {
        if (v.isValid()) {
            //            qDebug() << "Result of injectJson: " << v;
        }
    });
}

void WebEngineQtExecutor::executeApplicationScript(const QByteArray& script, const QUrl& /*sourceUrl*/) {

    //    qDebug() << "executeApplicationScript: " << script;

    d_ptr->myPage->runJavaScript(script, 0, [=](const QVariant& v) {
        if (v.isValid()) {
            //            qDebug() << "Result of executeApplicationScript: " << v;
        }

        Q_EMIT applicationScriptDone();
    });
}

void WebEngineQtExecutor::executeJSCall(const QString& method,
                                        const QVariantList& args,
                                        const IExecutor::ExecuteCallback& callback) {
    QByteArrayList stringifiedArgs;
    for (const QVariant& arg : args) {
        if (arg.type() == QVariant::List || arg.type() == QVariant::Map) {
            QJsonDocument doc = QJsonDocument::fromVariant(arg);
            stringifiedArgs += doc.toJson(QJsonDocument::Compact);
        } else {
            stringifiedArgs += '"' + arg.toString().toLocal8Bit() + '"';
        }
    }

    QByteArray code = QByteArray("__fbBatchedBridge.") + method.toLocal8Bit() + "(" + stringifiedArgs.join(',') + ");";

    //    qDebug() << "executeJSCall: " << code;

    d_ptr->myPage->runJavaScript(code, 0, [=](const QVariant& v) {
        if (v.isValid()) {
            //            qDebug() << "Result of executeJSCall: " << v;
        }

        QJsonDocument doc;
        if (v != "undefined") {
            doc = QJsonDocument::fromVariant(v);
        }
        callback(doc);
    });
}

void WebEngineQtExecutor::init() {}

void WebEngineQtExecutor::resetConnection() {}
