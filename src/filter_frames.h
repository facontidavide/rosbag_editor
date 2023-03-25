#ifndef FILTER_FRAMES_H
#define FILTER_FRAMES_H

#include <QDialog>
#include <rosbag/bag.h>
#include <rosbag/view.h>

namespace Ui {
class FilterFrames;
}

class FilterFrames : public QDialog
{
  Q_OBJECT

 public:
  using FramesSet = std::set<std::pair<std::string,std::string>>;

  explicit FilterFrames(const rosbag::Bag& bag,
                        const std::string& tf,
                        FramesSet &filtered_frames,
                        QWidget *parent = nullptr );

  ~FilterFrames();

 private slots:

  void on_buttonBox_accepted();

 private:
  Ui::FilterFrames *ui;
  const rosbag::Bag& _bag;
  FramesSet& _frames_to_filter;
};

#endif // FILTER_FRAMES_H
