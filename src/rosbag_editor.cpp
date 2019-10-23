#include "rosbag_editor.h"
#include "ui_rosbag_editor.h"

#include <topic_tools/shape_shifter.h>
#include <tf/tfMessage.h>
#include <tf2_msgs/TFMessage.h>

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
#include <QLineEdit>
#include <QCheckBox>
#include <QProgressDialog>
#include "filter_frames.h"

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
    ui->radioNoCompression->setChecked(true);
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
        ui->tableWidgetOutput->setRowCount(0);
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


    std::map<QString,QString> connections_ordered;

    for(std::size_t i = 0; i < connections.size(); i++ )
    {
      const rosbag::ConnectionInfo* connection = connections[i];
     connections_ordered.insert( std::make_pair(QString::fromStdString(connection->topic),
                                                QString::fromStdString(connection->datatype) ) );
    }

    int row = 0;
    for(const auto conn: connections_ordered )
    {
        auto type_item = new QTableWidgetItem( conn.second );
        QFont font = type_item->font();
        font.setPointSize(8);
        font.setItalic(true);
        type_item->setFont(font);

        ui->tableWidgetInput->setItem(row, 0, new QTableWidgetItem( conn.first ) );
        ui->tableWidgetInput->setItem(row, 1, type_item);
        row++;
    }
    changeEnabledWidgets();
}

void RosbagEditor::changeEnabledWidgets()
{
  ui->pushButtonMove->setEnabled( ui->tableWidgetInput->selectionModel()->selectedRows().count() > 0 );
  bool output = (ui->tableWidgetInput->rowCount() > 0);
  ui->tableWidgetOutput->setEnabled( output );
  ui->dateTimeOutputBegin->setEnabled( output );
  ui->dateTimeOutputEnd->setEnabled( output );
  ui->pushButtonSave->setEnabled( output );

  bool contains_tf = !ui->tableWidgetInput->findItems( "/tf", Qt::MatchExactly ).empty();
  ui->pushButtonFilterTF->setEnabled( ui->checkBoxFilterTF->isChecked() && contains_tf );
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

        if( ui->tableWidgetOutput->findItems( topic_name, Qt::MatchExactly ).isEmpty() )
        {
          int row = ui->tableWidgetOutput->rowCount();
          ui->tableWidgetOutput->setRowCount(row+1);
          ui->tableWidgetOutput->setItem(row, 0, new QTableWidgetItem(topic_name) );
          QLineEdit* topic_editor = new QLineEdit(ui->tableWidgetOutput);
          ui->tableWidgetOutput->setCellWidget(row, 1, topic_editor);
        }
    }

    ui->tableWidgetInput->selectionModel()->clearSelection();

    ui->pushButtonSave->setEnabled( ui->tableWidgetOutput->rowCount() );

    ui->dateTimeOutputBegin->setDateTimeRange(ui->dateTimeInputBegin->dateTime(),
                                              ui->dateTimeInputEnd->dateTime() );
    ui->dateTimeOutputBegin->setDateTime(ui->dateTimeInputBegin->dateTime());

    ui->dateTimeOutputEnd->setDateTimeRange(ui->dateTimeInputBegin->dateTime(),
                                            ui->dateTimeInputEnd->dateTime() );
    ui->dateTimeOutputEnd->setDateTime(ui->dateTimeInputEnd->dateTime());

    changeEnabledWidgets();
}

void RosbagEditor::on_tableWidgetInput_itemSelectionChanged()
{
    QItemSelectionModel *select = ui->tableWidgetInput->selectionModel();
    ui->pushButtonMove->setEnabled( select->hasSelection() );
}

void RosbagEditor::on_tableWidgetOutput_itemSelectionChanged()
{
    QItemSelectionModel *select = ui->tableWidgetOutput->selectionModel();
    ui->pushButtonRemove->setEnabled( select->hasSelection() );
}

void RosbagEditor::on_pushButtonRemove_pressed()
{
    QModelIndexList indexes;
    while((indexes = ui->tableWidgetOutput->selectionModel()->selectedIndexes()).size())
    {
        ui->tableWidgetOutput->model()->removeRow(indexes.first().row());
    }

    ui->tableWidgetOutput->sortItems(0);

    changeEnabledWidgets();
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

    if( ui->radioNoCompression->isChecked()){
      out_bag.setCompression( rosbag::CompressionType::Uncompressed );
    }
    else if( ui->radioLZ4->isChecked()){
      out_bag.setCompression( rosbag::CompressionType::LZ4 );
    }
    else if( ui->radioBZ2->isChecked()){
      out_bag.setCompression( rosbag::CompressionType::BZ2 );
    }

    std::vector<std::string> input_topics;
    std::map<std::string,std::string> topis_renamed;

    for(int row = 0; row < ui->tableWidgetOutput->rowCount(); ++row)
    {
        std::string name =  ui->tableWidgetOutput->item(row,0)->text().toStdString();
        QLineEdit* line_edit = qobject_cast<QLineEdit*>(ui->tableWidgetOutput->cellWidget(row, 1));
        std::string renamed = line_edit->text().toStdString();
        input_topics.push_back( name );
        if( renamed.empty())
        {
          topis_renamed.insert( std::make_pair(name,name));
        }
        else{
          topis_renamed.insert( std::make_pair(name,renamed));
        }
    }

    double begin_time = std::floor(-0.001 + 0.001*static_cast<double>(ui->dateTimeOutputBegin->dateTime().toMSecsSinceEpoch()));
    double end_time   = std::ceil(  0.001 +  0.001*static_cast<double>(ui->dateTimeOutputEnd->dateTime().toMSecsSinceEpoch()));


    rosbag::View bag_view( _bag, rosbag::TopicQuery(input_topics),
                          ros::Time( begin_time ), ros::Time( end_time ) );

    QProgressDialog progress_dialog;
    progress_dialog.setLabelText("Loading... please wait");
    progress_dialog.setWindowModality( Qt::ApplicationModal );
    progress_dialog.show();

    int msg_count = 0;
    progress_dialog.setRange(0, bag_view.size()-1);

    for(const rosbag::MessageInstance& msg: bag_view)
    {
      if( msg_count++ %100 == 0)
      {
        progress_dialog.setValue( msg_count );
        QApplication::processEvents();
      }

      const auto& name = topis_renamed.find(msg.getTopic())->second;

      auto removeTransform = [&](auto tf)
      {
        for (int i=0; i < tf->transforms.size(); i++)
        {
          auto frame = std::make_pair(tf->transforms[i].header.frame_id,
                                      tf->transforms[i].child_frame_id);
          if( _filtered_frames.count(frame) )
          {
            tf->transforms.erase( tf->transforms.begin() + i );
            i--;
          }
        }
        out_bag.write( name, msg.getTime(), tf, msg.getConnectionHeader());
      };


      if( msg.getTopic() == "/tf" && _filtered_frames.size() > 0)
      {
        tf::tfMessage::Ptr tf = msg.instantiate<tf::tfMessage>();
        if (tf)
        {
          removeTransform(tf);
        }

        tf2_msgs::TFMessage::Ptr tf2 = msg.instantiate<tf2_msgs::TFMessage>();
        if (tf2)
        {
          removeTransform(tf2);
        }
      }
      else{
        out_bag.write( name, msg.getTime(), msg, msg.getConnectionHeader());
      }
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

void RosbagEditor::on_checkBoxFilterTF_toggled(bool checked)
{
  bool contains_tf = !ui->tableWidgetInput->findItems( "/tf", Qt::MatchExactly ).empty();
  ui->pushButtonFilterTF->setEnabled( checked && contains_tf );
}

void RosbagEditor::on_pushButtonFilterTF_pressed()
{
  FilterFrames dialog(_bag, _filtered_frames, this);
  dialog.exec();

}
