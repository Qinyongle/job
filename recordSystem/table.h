#ifndef TABLE_H
#define TABLE_H

#include <QWidget>
#include <common.h>
namespace Ui {
class table;
}

class table : public QWidget
{
    Q_OBJECT

public:
    explicit table(QWidget *parent = nullptr);
    ~table();

    void rightClickInit(void);
    void tableViewInit(void);
    void drawTable(void);

    QStandardItemModel *model;
    QMenu *m_actMenu;
    QAction *m_actDevRefresh;
    QAction *actDelet;
    QItemSelectionModel *selectModel;
    QDateTime *datetime;
    uint32_t rowCount=0;
    uint32_t colCount=0;

private:
    Ui::table *ui;

private slots:
    void on_tableViewMenu(const QPoint &pos);

};

#endif // TABLE_H
