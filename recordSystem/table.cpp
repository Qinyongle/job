#include "table.h"
#include "ui_table.h"

table::table(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::table)
{
    ui->setupUi(this);

    myAudio =new Audio();
    myAudioThread=new QThread();
    myAudio->moveToThread(myAudioThread);
    connect(myAudio,&Audio::recordStart,myAudio,&Audio::audioRecord);
    myAudioThread->start();

    rightClickInit();

    tableViewInit();

    drawTable();
}

table::~table()
{
    myAudioThread->deleteLater();
    myAudio->deleteLater();
    delete ui;

}


void table::rightClickInit()
{
     datetime=new QDateTime;
     m_actMenu = new QMenu(ui->tableView);
     m_actMenu->setStyleSheet("border:1px solid #4d738b;");
     m_actDevRefresh = new QAction("addrow", ui->tableView);
     connect(m_actDevRefresh, &QAction::triggered, [this]()
       {
              model->insertRow( ui->tableView->currentIndex().row()+1);//在下一行插入
              *datetime=QDateTime::currentDateTime();
              QString monthDay=datetime->toString("yyyy/MM/dd");
              QString time=datetime->toString("hh:mm");

              model->setItem(ui->tableView->currentIndex().row()+1,0,new QStandardItem(monthDay));
              model->setItem(ui->tableView->currentIndex().row()+1,1,new QStandardItem(time));
       });

    actRecord=new QAction("record",ui->tableView);
    connect(actRecord,&QAction::triggered,[this]()
    {
        emit myAudio->recordStart();
    });

     actDelet=new QAction("delete",ui->tableView);
     connect(actDelet,&QAction::triggered,[this]()
     {
         model->removeRow(ui->tableView->currentIndex().row());
     });



       m_actMenu->addAction(m_actDevRefresh);
       m_actMenu->addAction(actDelet);
       m_actMenu->addAction(actRecord);

       ui->tableView->setContextMenuPolicy(Qt::CustomContextMenu);
       connect(ui->tableView, &QTableView::customContextMenuRequested,
               this, &table::on_tableViewMenu);
}

void table::tableViewInit()
{
    model = new QStandardItemModel;
    ui->tableView->setModel(model);

    ui->tableView->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    ui->tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    ui->tableView->setAlternatingRowColors(true);

    //设置最后一栏自适应长度
    ui->tableView->horizontalHeader()->setStretchLastSection(true);
    ui->tableView->resizeColumnsToContents();
    ui->tableView->resizeRowsToContents();

}

void table::drawTable()
{
    QSettings *settings;
    settings = new QSettings("E:/Qyl_Program/recordSystem/config/item.ini", QSettings::IniFormat);
    settings->setIniCodec(QTextCodec::codecForName("UTF-8"));
    QStringList list= settings->allKeys();
    QString str;

    foreach(str,list)
    {
        model->setHorizontalHeaderItem(colCount++, new QStandardItem(settings->value(str).toString()));
    }

    model->setRowCount(1);
}

void table::on_tableViewMenu(const QPoint &pos)
{

    //m_actMenu->exec(mapToGlobal(pos));//绝对位置
    m_actMenu->exec(cursor().pos());//相对位置

}
