#include "rosbag_editor.h"
#include "ui_rosbag_editor.h"

#include <topic_tools/shape_shifter.h>

#include <QDir>
#include <QString>
#include <QFileDialog>
#include <QDateTime>
#include <QSettings>
#include <QFont>
#include <QDateTimeEdit>
#include <QMessageBox>
#include <QItemSelectionModel>
#include <QPushButton>
#include <QListWidget>
#include <QStatusBar>
#include <QFileInfo>

RosbagEditor::RosbagEditor(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::RosbagEditor)
{
    QApplication::setWindowIcon(QIcon("://rosbag_editor.png"));
    QSettings settings;

    restoreGeometry(settings.value("RosbagEditor/geometry").toByteArray());
    restoreState(settings.value("RosbagEditor/windowState").toByteArray());

    _previous_load_path = settings.value("RosbagEditor/prevLoadPath", QDir::currentPath()).toString();
    _previous_save_path = settings.value("RosbagEditor/prevSavePath", _previous_load_path).toString();

    ui->setupUi(this);
}

void RosbagEditor::closeEvent(QCloseEvent *event)
{
    QSettings settings;
    settings.setValue("RosbagEditor/geometry", saveGeometry());
    settings.setValue("RosbagEditor/windowState", saveState());
    QMainWindow::closeEvent(event);
}

RosbagEditor::~RosbagEditor()
{
    delete ui;
}

void RosbagEditor::on_pushButtonLoad_pressed()
{
    QString filename = QFileDialog::getOpenFileName(this,
        tr("Open Rosbag"), _previous_load_path, tr("Rosbag Files (*.bag)"));

    if( filename.isEmpty())
    {
        return;
    }

    try{
        ui->tableWidgetInput->setRowCount(0);
        ui->listWidgetOutput->clear();
        _bag.close();
        _bag.open( filename.toStdString() );
    }
    catch( rosbag::BagException&  ex)
    {
        QMessageBox::warning(this, "Error opening the rosbag",
                             tr("rosbag::open thrown an exception: %1\n").arg( ex.what()) );
        return;
    }

    QSettings settings;
    settings.setValue("RosbagEditor/prevLoadPath", QFileInfo(filename).absolutePath() );

    ui->statusbar->showMessage( tr("File loaded: %1").arg(filename));

    _loade_filename = filename;

    rosbag::View bag_view ( _bag );
    auto connections = bag_view.getConnections();

    ui->tableWidgetInput->setRowCount(connections.size());
    ui->tableWidgetInput->setColumnCount(2);
    ui->tableWidgetInput->setEnabled(true);

    QDateTime datetime_begin = QDateTime::fromMSecsSinceEpoch( bag_view.getBeginTime().toSec()*1000 );
    ui->dateTimeInputBegin->setDateTime(datetime_begin);

    QDateTime datetime_end = QDateTime::fromMSecsSinceEpoch( bag_view.getEndTime().toSec()*1000 );
    ui->dateTimeInputEnd->setDateTime(datetime_end);

    for(std::size_t i = 0; i < connections.size(); i++ )
    {
        const rosbag::ConnectionInfo* connection = connections[i];
        QString topic_name =  QString::fromStdString(connection->topic);
        QString datatype   =  QString::fromStdString(connection->datatype);

        auto type_item = new QTableWidgetItem( datatype );
        QFont font = type_item->font();
        font.setPointSize(8);
        font.setItalic(true);
        type_item->setFont(font);

        ui->tableWidgetInput->setItem(i, 0, new QTableWidgetItem( topic_name) );
        ui->tableWidgetInput->setItem(i, 1, type_item);
    }
}

void RosbagEditor::on_pushButtonMove_pressed()
{
    QModelIndexList selected_input  = ui->tableWidgetInput->selectionModel()->selectedRows();
    if( selected_input.count() == 0)
    {
        return;
    }

    for(int i=0; i < selected_input.count(); i++)
    {
        QModelIndex index = selected_input.at(i);
        QTableWidgetItem* item = ui->tableWidgetInput->item( index.row(), 0);
        QString topic_name = item->text();

        if( ui->listWidgetOutput->findItems( topic_name, Qt::MatchExactly ).isEmpty() )
        {
            ui->listWidgetOutput->addItem(topic_name);
        }
    }

    QModelIndexList selected_output = ui->listWidgetOutput->selectionModel()->selectedRows();
    ui->pushButtonMove->setEnabled( false );

    ui->listWidgetOutput->setEnabled( true );
    ui->pushButtonSave->setEnabled( ui->listWidgetOutput->count() );

    ui->dateTimeOutputBegin->setEnabled( true );
    ui->dateTimeOutputBegin->setDateTimeRange(ui->dateTimeInputBegin->dateTime(),
                                              ui->dateTimeInputEnd->dateTime() );
    ui->dateTimeOutputBegin->setDateTime(ui->dateTimeInputBegin->dateTime());

    ui->dateTimeOutputEnd->setEnabled( true );
    ui->dateTimeOutputEnd->setDateTimeRange(ui->dateTimeInputBegin->dateTime(),
                                            ui->dateTimeInputEnd->dateTime() );
    ui->dateTimeOutputEnd->setDateTime(ui->dateTimeInputEnd->dateTime());

}

void RosbagEditor::on_tableWidgetInput_itemSelectionChanged()
{
    QItemSelectionModel *select = ui->tableWidgetInput->selectionModel();
    ui->pushButtonMove->setEnabled( select->hasSelection() );
}

void RosbagEditor::on_listWidgetOutput_itemSelectionChanged()
{
    QItemSelectionModel *select = ui->listWidgetOutput->selectionModel();
    ui->pushButtonRemove->setEnabled( select->hasSelection() );
}

void RosbagEditor::on_pushButtonRemove_pressed()
{
    QModelIndexList indexes;
    while((indexes = ui->listWidgetOutput->selectionModel()->selectedIndexes()).size())
    {
        ui->listWidgetOutput->model()->removeRow(indexes.first().row());
    }

    ui->pushButtonSave->setEnabled( ui->listWidgetOutput->count() );
    ui->listWidgetOutput->sortItems();
}

void RosbagEditor::on_pushButtonSave_pressed()
{
    QString filename = QFileDialog::getSaveFileName(this, "Save Rosbag",
                                                    _previous_save_path,
                                                    tr("Rosbag Files (*.bag)"));
    if( filename.isEmpty())
    {
        return;
    }
    if (QFileInfo(filename).suffix() != "bag")
    {
        filename.append(".bag");
    }

    if( filename == _loade_filename)
    {
        QMessageBox::warning(this, "Wrong file name",
                             "You can not overwrite the input file. Choose another name or directory.",
                             QMessageBox::Ok);
        return;
    }

    QSettings settings;
    settings.setValue("RosbagEditor/prevSavePath", QFileInfo(filename).absolutePath() );

    rosbag::Bag out_bag;

    out_bag.open(filename.toStdString(), rosbag::bagmode::Write);

    std::vector<std::string> topics;

    for(int i = 0; i < ui->listWidgetOutput->count(); ++i)
    {
        QListWidgetItem* item = ui->listWidgetOutput->item(i);
        topics.push_back( item->text().toStdString() );
    }

    double begin_time = std::floor(-0.001 + 0.001*static_cast<double>(ui->dateTimeOutputBegin->dateTime().toMSecsSinceEpoch()));
    double end_time   = std::ceil(  0.001 +  0.001*static_cast<double>(ui->dateTimeOutputEnd->dateTime().toMSecsSinceEpoch()));


    rosbag::View bag_view( _bag, rosbag::TopicQuery(topics),
                          ros::Time( begin_time ), ros::Time( end_time ) );

    for(const rosbag::MessageInstance& msg: bag_view)
    {
        out_bag.write( msg.getTopic(), msg.getTime(), msg);
    }
    out_bag.close();

    int ret = QMessageBox::question(this, "Done", "New robag succesfully created. Do you want to close the application?",
                                    QMessageBox::Cancel | QMessageBox::Close, QMessageBox::Close );
    if( ret == QMessageBox::Close )
    {
        this->close();
    }
}

void RosbagEditor::on_dateTimeOutputEnd_dateTimeChanged(const QDateTime &dateTime)
{
    if( ui->dateTimeOutputBegin->dateTime() > dateTime  )
    {
        ui->dateTimeOutputBegin->setDateTime( dateTime );
    }
}

void RosbagEditor::on_dateTimeOutputBegin_dateTimeChanged(const QDateTime &dateTime)
{
    if( ui->dateTimeOutputEnd->dateTime() < dateTime  )
    {
        ui->dateTimeOutputEnd->setDateTime( dateTime );
    }
}
