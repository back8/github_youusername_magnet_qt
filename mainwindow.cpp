#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "QTableView"
#include "QStandardItemModel"
#include <qdesktopservices.h>
#include <QLabel>
#include <QPixmap>
#include <QClipboard>
#include "httpclient.h"

static const QString RULE_URL = "https://gitee.com/zvj88888888/magnet_qt/raw/master/rule.json";

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{

    ui->setupUi(this);



    progressCircle = new ProgressCircle();

    progressCircle->setFixedSize(30, 30);
    progressCircle->setMaximumHeight(30);
    progressCircle->setHidden(true);
    QHBoxLayout *layout = new QHBoxLayout();
    layout->setAlignment(Qt::AlignCenter);
    layout->addWidget(progressCircle);
    ui->loading->setLayout(layout);


    connect(webView,&QWebEngineView::loadFinished,[this](int){
        qDebug()<<"loadFinished";
        webView->page()->runJavaScript("document.documentElement.outerHTML",QWebEngineScript::ApplicationWorld,[this](const QVariant& data) {
            qDebug()<<"123123:"<<data.toString();
            sideModel* object = jsonModelList[currentListSelect];
            resultList = queryHTML(data.toString(),object);

            reloadTableData(resultList);
            //加载完毕,隐藏loading
            progressCircle->setHidden(true);
        });
//        webView->page()->mainFrame()->documentElement().evaluateJavaScript("...");
    });


    currentPage = 1;

    initTableView();
    initListTableView();

    QPixmap pix(":/img/icon512.png");
    ui->label_pix->setPixmap(pix.scaled(100,100,Qt::KeepAspectRatio));


    QFileInfo file("rule.json");
//    QFile openFile(file.absoluteFilePath());
//    if(file.isFile()){
//        openFile.remove();
//    }

    HttpClient(RULE_URL).debug(true).success([=](const QString &response) {

    }).fail([](const QString &error, int errorCode) {
        qDebug() << error << errorCode;
    }).download(file.absoluteFilePath());
}
void MainWindow::initTableView(){
    XTTableView *tableview = ui->tableView;
    tableview->setSelectionBehavior(QAbstractItemView::SelectRows);
    tableview->setSelectionMode(QAbstractItemView::SingleSelection);
    tableview->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tableview->setAlternatingRowColors(true);


    connect(tableview,SIGNAL(leftClicked(const QModelIndex &)),this,SLOT(testSlot()));

    QStandardItemModel *model = new QStandardItemModel();
    tableview->setModel(model);//来使用model
//    model->setHeaderData(0,Qt::Horizontal,"ID");//设置表的column名称；
    model->setColumnCount(3);    //列

    //添加数据
//    for(int j=0;j<15;j++)
//    {
//         //写id
//        QStandardItem *itemID = new QStandardItem("hello"+QString::number(j));//QString::number(j)));
//        model->setItem(j,0,itemID);

//        QStandardItem *item2 = new QStandardItem("22222"+QString::number(j));
//        model->setItem(j,1,item2);

//        QPushButton * m_button = new QPushButton("打开");

//                        //触发下载按钮的槽函数
//        connect(m_button, SIGNAL(clicked(bool)), this, SLOT(clickDownloadButton())); //
//        m_button->setProperty("row", j);  //为按钮设置row属性
//        m_button->setProperty("fileName", QString::number(j));  //为按钮设置fileName属性
//        //m_button->setProperty("column", col)

//       ui->tableView->setIndexWidget(model->index(model->rowCount() - 1,3),m_button);

//    }
    tableview->setColumnWidth(0,tableview->size().width()-160);
    tableview->setColumnWidth(1,80);
    tableview->setColumnWidth(2,80);
//    tableview->setColumnWidth(3,80);
    tableview->showMaximized();


    QHeaderView * header = tableview->verticalHeader();//ui->fileTree->header();
//    header->setSectionResizeMode(QHeaderView::ResizeToContents);
//    header->setStretchLastSection(false);
    header->setSectionResizeMode(0, QHeaderView::Stretch);
    header->setSectionResizeMode(1, QHeaderView::Fixed);
    header->setSectionResizeMode(2, QHeaderView::Fixed);

    tableview->setContextMenuPolicy(Qt::CustomContextMenu);  //少这句，右键没有任何反应的。

    rightMenu = new QMenu;

    copyAction = new QAction("复制",this);
    downloadAction = new QAction("下载",this);

    rightMenu->addAction(copyAction);
    rightMenu->addAction(downloadAction);

    connect(tableview,SIGNAL(customContextMenuRequested(QPoint)),this,SLOT(clicked_rightMenu(QPoint)));
    connect(rightMenu, SIGNAL(triggered(QAction *)),
            this, SLOT(onShowOrHideColumn(QAction *)));

}

void MainWindow::initListTableView(){
    XTTableView *listTableView = ui->listTableView;
    listTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    listTableView->setSelectionMode(QAbstractItemView::SingleSelection);
    listTableView->setEditTriggers(QAbstractItemView::NoEditTriggers);

    connect(listTableView,SIGNAL(leftClicked(const QModelIndex &)),this,SLOT(clickListTableView()));

    QStandardItemModel *model = new QStandardItemModel();
    listTableView->setModel(model);//来使用model
    model->setColumnCount(1);    //列

    QFileInfo file("rule.json");
    if(!file.isFile()){
        qDebug() <<"no File!";
        if(QFile::copy(":/rule","rule.json")){
            qDebug() <<"copy File!";
        }
    }
    qDebug() <<"File path:"<<file.absoluteFilePath();
    QFile openFile(file.absoluteFilePath());

    if(!openFile.open(QIODevice::ReadOnly)) {
        qDebug() << "File open error";
    } else {
        qDebug() <<"File open!";
    }

    QString allData = openFile.readAll();
    openFile.close();


    QJsonParseError json_error;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(allData.toUtf8(), &json_error);
    qDebug() << "json:"<<jsonDoc;
    if(jsonDoc.isNull() || (json_error.error != QJsonParseError::NoError))
    {
            qDebug() << "json error!"<<json_error.errorString();
            return;
     }

    QJsonArray array = jsonDoc.array();


    for(int i = 0; i < array.count(); i++)
    {

        QJsonObject object = array[i].toObject();
        QStandardItem *item = new QStandardItem(object.value("site").toString());
        model->setItem(i,0,item);

        sideModel *jsonmodel = new sideModel();
        jsonmodel->site = object.value("site").toString();
        jsonmodel->magnet = object.value("magnet").toString();
        jsonmodel->group = object.value("group").toString();
        jsonmodel->name   = object.value("name").toString();
        jsonmodel->size   = object.value("size").toString();
        jsonmodel->count  = object.value("count").toString();
        jsonmodel->source = object.value("source").toString();
        jsonModelList << jsonmodel;
    }
//    qDebug()<<"jsonmodel:"<<jsonmodel->magnet;
//    delete(jsonmodel);

    listTableView->setColumnWidth(0,150);
    listTableView->selectRow(0);
    currentListSelect = 0;

}
//源网站列表点击
void MainWindow::clickListTableView(){
//    qDebug() << "list_row:" << ui->listTableView->currentIndex().row();
    currentListSelect = ui->listTableView->currentIndex().row();

    //搜索框不为空就直接切换源站点搜索
    if(!ui->searchText->text().isEmpty()){
        on_searchButton_clicked();
    }
}

void MainWindow::clicked_rightMenu(const QPoint &pos)  //右键信号槽函数
{
    rightMenu->exec(QCursor::pos());

    qDebug() << pos;
}

void MainWindow::onShowOrHideColumn(QAction *action)
{
//    qDebug() << action;
//    qDebug() << "menu_row:"<<ui->tableView->currentIndex().row();
    sideModel * model = resultList[ui->tableView->currentIndex().row()];
    if(action->text().compare("复制")==0){

        QClipboard *clipboard = QApplication::clipboard();  //获取系统剪贴板指针
        QString originalText = clipboard->text();           //获取剪贴板上文本信息

        clipboard->setText(model->magnet);                  //设置剪贴板内容

    }else {
        QDesktopServices::openUrl(QUrl(model->magnet));
    }

}

//搜索按钮点击事件
void MainWindow::on_searchButton_clicked(){

    resultList.clear();

    if(ui->tableView->model() != nullptr){
        delete(ui->tableView->model());
        ui->tableView->setModel(nullptr);
    }
//    qDebug() << "search_row:" << currentListSelect;
    sideModel* object = jsonModelList[currentListSelect];

    if(jsonModelList.length()<=0){
        qDebug() << "源网站对象为空";

        return;
    }

    if(ui->searchText->text().isEmpty()){
        qDebug() << "搜索关键字为空";
        return;
    }
    //显示Loading
    progressCircle->setHidden(false);

    QString string = object->source;//object.value("source").toString();

    QString str_page = string.replace(QRegExp("PPP"), QString::number(currentPage));

    QByteArray keyword_utf8 = ui->searchText->text().toUtf8();
    QByteArray byteArrayPercentEncoded = keyword_utf8.toPercentEncoding();
    QString encode_keyword =  QString(byteArrayPercentEncoded);
    QString url_str = string.replace(QRegExp("XXX"),encode_keyword);
    qDebug()<<"search URL:"<<url_str;
    qDebug()<<"-------";

    webView->setUrl(QUrl(url_str));

//    HttpClient(QUrl(url_str).fromPercentEncoding(url_str.toUtf8())).debug(true).success([this,object](const QString &response) {
//        qDebug() << response;
//        resultList = queryHTML(response,object);

//        reloadTableData(resultList);
//    }).fail([](const QString &error, int errorCode) {
//        qDebug() << error << errorCode;
//    }).get();

}

void MainWindow::htmlFinished(QNetworkReply *reply){

    sideModel * object = jsonModelList[currentListSelect];

    //取出域名判断是不是最后点击的网站
    QString reply_url = reply->url().toString().split(QRegExp("://"))[1].split(QRegExp("/"))[0];
    QString current_url = object->source.split(QRegExp("://"))[1].split(QRegExp("/"))[0];

//    qDebug()<<"reply_url:"<<reply_url;
//    qDebug()<<"current_url:"<<current_url;

    if(reply_url.compare(current_url) == 0){

        QString responseData = reply->readAll();
        qDebug()<<"-------";
//        qDebug()<<"html_URL:"<< reply->url();
        qDebug()<<"htmlFinished:"<< responseData.length();
        qDebug()<<"-------";
        resultList = queryHTML(responseData,object);

        reloadTableData(resultList);
    }else {
        qDebug()<<"不是当前选中,路过结果!";
    }



}

QList<sideModel*> MainWindow::queryHTML(const QString &html, sideModel * model) {

    QList<sideModel*> list;

    QString group = model->group;

    QString query_magnet = group + clearDot(model->magnet);
    QString query_name = group + clearDot(model->name);
    QString query_size = group + clearDot(model->size);
    QString query_count = group + clearDot(model->count);

//    qDebug()<<"magnet:"<<query_magnet;
//    qDebug()<<"name:"<<query_name;
//    qDebug()<<"size:"<<query_size;
//    qDebug()<<"count:"<<query_count;
//    qDebug()<<"-------";

    htmlParserCtxtPtr ctxt = htmlNewParserCtxt();
    htmlDocPtr htmlDoc = htmlCtxtReadMemory(ctxt, html.toUtf8().constData(), (int)strlen(html.toUtf8().constData()), "", NULL, HTML_PARSE_NOBLANKS | HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING | HTML_PARSE_NONET | HTML_PARSE_NODEFDTD);
    xmlXPathContextPtr context = xmlXPathNewContext ( htmlDoc );

    xmlXPathObjectPtr result_magnet = xmlXPathEvalExpression ((const xmlChar*)query_magnet.toUtf8().constData(), context);
    xmlXPathObjectPtr result_name = xmlXPathEvalExpression ((const xmlChar*)query_name.toUtf8().constData(), context);
    xmlXPathObjectPtr result_size = xmlXPathEvalExpression ((const xmlChar*)query_size.toUtf8().constData(), context);
    xmlXPathObjectPtr result_count = xmlXPathEvalExpression ((const xmlChar*)query_count.toUtf8().constData(), context);

    xmlXPathFreeContext (context);


    if (result_magnet == nullptr) {
        qDebug()<<"Invalid XQuery ?";
    }
    else {
        xmlNodeSetPtr node_magnet = result_magnet->nodesetval;
        xmlNodeSetPtr node_name = result_name->nodesetval;
        xmlNodeSetPtr node_size = result_size->nodesetval;
        xmlNodeSetPtr node_count = result_count->nodesetval;

        if ( !xmlXPathNodeSetIsEmpty ( node_magnet ) ) {
            for (int i = 0; i < node_magnet->nodeNr; i++ ) {
                xmlNodePtr  nodePtr_magnet = node_magnet->nodeTab[i];
                xmlNodePtr  nodePtr_name = node_name->nodeTab[i];
                xmlNodePtr  nodePtr_size = node_size->nodeTab[i];
                xmlNodePtr  nodePtr_count = node_count->nodeTab[i];

                sideModel * result_model = new sideModel();

                QString magnet = QString::fromUtf8((char*)nodePtr_magnet->children->content);
                QString size = QString::fromUtf8((char*)nodePtr_size->children->content);
                QString count = QString::fromUtf8((char*)nodePtr_count->children->content);

                QString name = "";
                if(!QString::fromUtf8((char*)nodePtr_name->children->content).isEmpty()){
                    name = name + QString::fromUtf8((char*)nodePtr_name->children->content);
                }

                if(nodePtr_name->children->next != nullptr){
                    name = name + QString::fromUtf8((char*)nodePtr_name->children->next->content);
                }

                if(!QString::fromUtf8((char*)nodePtr_name->content).isEmpty()){
                    name = name + QString::fromUtf8((char*)nodePtr_name->content);
                }

                if(!QString::fromUtf8((char*)nodePtr_name->last->content).isEmpty()){
                    name = name + QString::fromUtf8((char*)nodePtr_name->last->content);
                }
                if(name.isEmpty()){
                    name = ui->searchText->text();
                }

                result_model->magnet    = clearMagnet(magnet);
                result_model->name      = clearSpace(name);
                result_model->site      = size;
                result_model->count     = count;

//                qDebug()<<"magnet:"<<magnet;
//                qDebug()<<"name:"<<name;
//                qDebug()<<"size:"<<size;
//                qDebug()<<"count:"<<count;
//                qDebug()<<"-------";

                list << result_model;

            }
        }

        xmlXPathFreeObject (result_magnet);
        xmlXPathFreeObject (result_name);
        xmlXPathFreeObject (result_size);
        xmlXPathFreeObject (result_count);
    }

    qDebug()<<"xPath结果:"<<list.length();
    return list;
}
QString MainWindow::clearMagnet(QString str){
    QString clearStr ;

    clearStr = clearSpace(str);

    QString pattern("magnet:?[^\"]+");
    QRegExp rx(pattern);

//    bool match = rx.exactMatch(str);
    if(rx.exactMatch(clearStr)){
        return clearStr;
    }

//    qDebug()<< "magnet:" << clearStr;
    if(clearStr.endsWith(".html")){
        clearStr = clearStr.replace(".html","");
//        qDebug()<< "magnet1:" << clearStr;
    }

    if(rx.exactMatch(clearStr)){
        return clearStr;
    }

    QStringList sections = clearStr.split(QRegExp("&"));
    if(sections.length()>1){
        clearStr = sections[0];
//        qDebug()<< "magnet2:" << clearStr;
    }

    if(rx.exactMatch(clearStr)){
        return clearStr;
    }

    if(clearStr.length()>=40){
        clearStr = "magnet:?xt=urn:btih:" + clearStr.mid(clearStr.length()-40,40);
//        qDebug()<< "magnet3:" << clearStr;
    }

    if(rx.exactMatch(clearStr)){
        return clearStr;
    }

//    qDebug()<< str << ":----:" << clearStr;
    return clearStr;
}
QString MainWindow::clearSpace(QString str){
    QString clearStr ;

    clearStr = str.replace(" ","");

//    qDebug()<< str << ":----:" << clearStr;
    return clearStr;
}

QString MainWindow::clearDot(QString str){
    QString clearStr ;
    if(str.startsWith(".")){
        clearStr = str.mid(1,str.length()-1);
    }else {
        clearStr = str;
    }
//    qDebug()<< str << ":----:" << clearStr;
    return clearStr;
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::reloadTableData(QList<sideModel*>list){

    QStandardItemModel *model = new QStandardItemModel();
    ui->tableView->setModel(model);//来使用model

//    model->setHeaderData(0,Qt::Horizontal,"ID");//设置表的column名称；
    model->setColumnCount(3);    //列

    //添加数据
    for(int j=0;j<list.length();j++)
    {
        sideModel * side = list[j];
         //写id
        QStandardItem *item_magnet = new QStandardItem(side->name);//QString::number(j)));
        model->setItem(j,0,item_magnet);

        QStandardItem *item_size = new QStandardItem(side->site);
        model->setItem(j,1,item_size);

        QStandardItem *item_count = new QStandardItem(side->count);
        model->setItem(j,2,item_count);



    }
    ui->tableView->setColumnWidth(0,ui->tableView->size().width()-162);
    ui->tableView->setColumnWidth(1,80);
    ui->tableView->setColumnWidth(2,80);
    ui->tableView->showMaximized();


    QHeaderView * header = ui->tableView->verticalHeader();//ui->fileTree->header();
//    header->setSectionResizeMode(QHeaderView::ResizeToContents);
//    header->setStretchLastSection(false);
    header->setSectionResizeMode(0, QHeaderView::Stretch);
    header->setSectionResizeMode(1, QHeaderView::Fixed);
    header->setSectionResizeMode(2, QHeaderView::Fixed);
}

void MainWindow::testSlot(){

    qDebug() << "row:" << ui->tableView->currentIndex().row() << "column:" << ui->tableView->currentIndex().column();
//    int row= ui->tableView->currentIndex().row();

//    QAbstractItemModel *model = ui->tableView->model ();

//    QModelIndex index = model->index(row,0);//选中行第一列的内容

//    QVariant data = model->data(index);

}

void MainWindow::clickDownloadButton(){

//    QPushButton *btn = (QPushButton *)sender();   //产生事件的对象
//    int row = btn->property("row").toInt();  //取得按钮的行号属性
//    QString openFileName = btn->property("fileName").toString();  //获取按钮的fileName属性，其他的可以自行添加。

//    sideModel * obj_model = resultList[row];

//    qDebug() << "click_magnet:" << model->magnet;
//    qDebug() << "button_row:"<<row << "row:" << ui->tableView->currentIndex().row() << "column:" << ui->tableView->currentIndex().column();

//    QString URL = "" + obj_model->magnet;
//    qDebug() << "url:" << URL;
//    QDesktopServices::openUrl(QUrl(URL));

}








