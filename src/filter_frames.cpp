#include "filter_frames.h"
#include <tf/tfMessage.h>
#include <tf2_msgs/TFMessage.h>
#include <QCheckBox>
#include <QProgressDialog>
#include <QTableWidget>
#include <QTableWidgetItem>
#include "ui_filter_frames.h"

FilterFrames::FilterFrames(const rosbag::Bag& bag,
                           const std::string& tf,
                           FilterFrames::FramesSet& filtered_frames,
                           QWidget* parent)
    : QDialog(parent), ui(new Ui::FilterFrames), _bag(bag), _frames_to_filter(filtered_frames)
{
  ui->setupUi(this);

  std::vector<std::string> topics = {tf};
  rosbag::View bag_view(bag, rosbag::TopicQuery(topics));

  QProgressDialog progress_dialog;
  progress_dialog.setLabelText("Loading... please wait");
  progress_dialog.setWindowModality(Qt::ApplicationModal);
  progress_dialog.show();

  int msg_count = 0;
  progress_dialog.setRange(0, bag_view.size() - 1);

  FramesSet frames;

  for (const rosbag::MessageInstance& msg : bag_view)
  {
    if (msg_count++ % 100 == 0)
    {
      progress_dialog.setValue(msg_count);
      QApplication::processEvents();
    }


    tf::tfMessage::ConstPtr tf = msg.instantiate<tf::tfMessage>();
    if (tf)
    {
      for (const auto& transform : tf->transforms)
      {
        frames.insert(std::make_pair(transform.header.frame_id,
                                     transform.child_frame_id));
      }
    }

    tf2_msgs::TFMessage::ConstPtr tf2 = msg.instantiate<tf2_msgs::TFMessage>();
    if (tf2)
    {
      for (const auto& transform : tf2->transforms)
      {
        frames.insert(std::make_pair(transform.header.frame_id,
                                     transform.child_frame_id));
      }
    }
  }

  for (const auto& frame : frames)
  {
    int row = ui->tableWidget->rowCount();
    ui->tableWidget->setRowCount(row + 1);
    ui->tableWidget->setItem(
        row, 1, new QTableWidgetItem(QString::fromStdString(frame.first)));
    ui->tableWidget->setItem(
        row, 2, new QTableWidgetItem(QString::fromStdString(frame.second)));
    QCheckBox* check = new QCheckBox(ui->tableWidget);
    check->setChecked( _frames_to_filter.count(frame) == 0 );
    ui->tableWidget->setCellWidget(row, 0, check);
  }

  ui->tableWidget->resizeColumnsToContents();
}

FilterFrames::~FilterFrames()
{
  delete ui;
}

void FilterFrames::on_buttonBox_accepted()
{
  for (int row = 0; row < ui->tableWidget->rowCount(); row++) {
    QCheckBox* check =
        qobject_cast<QCheckBox*>(ui->tableWidget->cellWidget(row, 0));

    if (check->isChecked() == false) {
      std::string parent = ui->tableWidget->item(row, 1)->text().toStdString();
      std::string child = ui->tableWidget->item(row, 2)->text().toStdString();
      _frames_to_filter.insert(std::make_pair(parent, child));
    }
  }
}
