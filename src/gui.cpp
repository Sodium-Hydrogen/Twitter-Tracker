#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QString>
#include <QtCore/QTimer>
#include <QtCore/QThread>
#include <QtCore/QUrl>
#include <QtGui/QImage>
#include <QtGui/QImageReader>
#include <QtWidgets/QApplication>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLayout>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QScrollBar>
#include <QtWidgets/QStyle>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QWidget>

//#include <thread>
#include <iostream>
#include <ctime>
#include "core.hpp"

using namespace std;

static QThread *update;

bool setup_file_system(QString = QDir::homePath());
void application_key_page(QWidget *, QString = (COREURL_BASEURL + COREURL_GETKEY).c_str());
void application_main_page(QWidget *);
void key_save_button(QWidget *, QLineEdit *);
void update_appliaction(QWidget *);


int main(int argc, char ** argv){
  QApplication app(argc, argv);
  QCoreApplication::setApplicationName("Twitter Stats");
  QWidget page;

  bool setup = setup_file_system();
  if(!setup){
    application_key_page(&page);
  }else{
    application_main_page(&page);
  }

  page.show();
  const int app_exec = app.exec();
  requests::global_cleanup();
  return app_exec;
}

bool setup_file_system(QString path){
  QString * return_path = new QString("");
  QDir program_home(path + "/.twitter_stats");
  CORE_APPPATH = program_home.absolutePath().toStdString();
  if(!program_home.exists()){
    program_home.cdUp();
    program_home.mkdir(".twitter_stats");
    program_home.cd(".twitter_stats");
    program_home.mkdir("imgs");
    return false;
  }
  try{
    json readKey(CORE_APPPATH + "/application_key.json", json::READ_FILE);
    readKey.parse_json();
    bool valid_key = verify_key(readKey.parsed["application_key"]);
    if(valid_key){
      CORE_APPKEY = readKey.parsed;
    }else{
      return false;
    }
  }catch(string e){
    return false;
  }

  return true;
}

void application_key_page(QWidget * parent, QString link){

  QLabel *label = new QLabel("Please <a href='" + link + "'>authorize twitter</a> and enter the key below:", parent);
  label->setTextFormat(Qt::RichText);
  label->setTextInteractionFlags(Qt::TextBrowserInteraction);
  label->setOpenExternalLinks(true);

  QWidget *keyRow = new QWidget;
  QLineEdit *keyEnter = new QLineEdit(parent);
  keyEnter->setMinimumWidth(400);
  QPushButton *keySave = new QPushButton("Save Key", parent);
  QObject::connect(keySave, &QPushButton::pressed, keyEnter,
    [parent, keyEnter](){key_save_button(parent, keyEnter);}
  );

  QVBoxLayout *layout = new QVBoxLayout;
  layout->addWidget(label);

  QHBoxLayout *keyLayout = new QHBoxLayout;
  keyLayout->addWidget(keyEnter);
  keyLayout->addWidget(keySave);
  keyRow->setLayout(keyLayout);

  layout->addWidget(keyRow);

  parent->setLayout(layout);
}

void key_save_button(QWidget * parent, QLineEdit * key){
  if(!save_key(key->text().toStdString(), CORE_APPPATH)){
    throw "Unable to save " + CORE_APPPATH + "/application_key.json";
  }
  application_main_page(parent);
}

void application_main_page(QWidget * parent){
  if(parent->isVisible()){
    qDeleteAll(parent->findChildren<QWidget *>("", Qt::FindDirectChildrenOnly));
    delete parent->layout();
  }

  QLabel * loading = new QLabel("Fetching info from twitter...", parent);
  QVBoxLayout * loading_layout = new QVBoxLayout;
  loading_layout->addWidget(loading);
  parent->setLayout(loading_layout);
  parent->updateGeometry();

  update = QThread::create([](){get_update(true);});
  QObject::connect(update, QThread::finished, parent, [parent](){update_appliaction(parent);});
  update->start();

  QTimer *updateApp = new QTimer;
  QObject::connect(updateApp, &QTimer::timeout, parent,[parent](){
    update = QThread::create([](){get_update(true);});
    QObject::connect(update, QThread::finished, parent, [parent](){update_appliaction(parent);});
    update->start();
  });
  updateApp->setInterval(1000*60*30);
  updateApp->start();

  if(parent->isVisible()){
    parent->update();
  }
}

void update_appliaction(QWidget * parent){
  qDeleteAll(parent->findChildren<QWidget *>("", Qt::FindDirectChildrenOnly));
  delete parent->layout();

  QWidget *user_area = new QWidget;
  QWidget *info_area = new QWidget;

  QWidget *username_area = new QWidget;
  QLabel *user_pic = new QLabel(parent);
  QLabel *user_name = new QLabel("Unable to connect.", parent);
  QLabel *user_handle = new QLabel(parent);

  QWidget *follower_area = new QWidget;
  QScrollArea *scroll_area = new QScrollArea;

  user_handle->setStyleSheet("QLabel {color: gray;}");

  QHBoxLayout *layout = new QHBoxLayout;
  QHBoxLayout *user_layout = new QHBoxLayout;
  QVBoxLayout *user_column = new QVBoxLayout;
  QVBoxLayout *username_layout = new QVBoxLayout;
  QVBoxLayout *follower_layout = new QVBoxLayout;

  user_name->setText(CORE_USER["name"].get_value().c_str());
  user_handle->setText(("@" + CORE_USER["screen_name"].get_value()).c_str());
  user_pic->setPixmap(QPixmap(QString::fromStdString(CORE_USER["saved_image_path"].get_value())));

  vector<string> times = CORE_FOLLOWERS["ids"].get_keys();
  for(int i = times.size()-1; i >= 0; i--){
    time_t unix_time = stoll(times[i]);
    string time_string = asctime(localtime(&unix_time));
    QLabel *time = new QLabel(time_string.substr(0, time_string.size()-1).c_str(), parent);
    follower_layout->addWidget(time);
    for(string key: CORE_FOLLOWERS["ids"][times[i]].get_keys()){
      for(int n = 0; n < CORE_FOLLOWERS["ids"][times[i]][key].size(); n++){
        json_element cur_ids = CORE_FOLLOWERS["ids"][times[i]][key];
        json_element user = CORE_FOLLOWERS["info"][cur_ids[n].get_value()];
        QWidget *follow_box = new QWidget;
        QWidget *info_box = new QWidget;
        QHBoxLayout *follow_layout = new QHBoxLayout;
        QVBoxLayout *info_layout = new QVBoxLayout;
        if(key == "added"){
          QLabel *added = new QLabel("+");
          added->setStyleSheet("QLabel{color: green;font-weight: bold;}");
          follow_layout->addWidget(added);
        }else{
          QLabel *removed = new QLabel("-");
          removed->setStyleSheet("QLabel{color: red;font-weight: bold;}");
          follow_layout->addWidget(removed);
        }
        if(user["name"].to_string() != "null"){
          QLabel *pic = new QLabel;
          pic->setPixmap(QPixmap(QString::fromStdString(user["saved_image_path"].get_value())));
          follow_layout->addWidget(pic);

          QLabel *name = new QLabel(user["name"].get_value().c_str());
          QLabel *handle = new QLabel(("@" + user["screen_name"].get_value()).c_str());
          if(user["account_exists"].to_string() == "false"){
            QLabel *no_account = new QLabel("Account no longer exists");
            no_account->setStyleSheet("QLabel{color: red;font-weight:bold;}");
            info_layout->addWidget(no_account);
          }
          handle->setStyleSheet("QLabel{color: gray;}");
          info_layout->addWidget(name);
          info_layout->addWidget(handle);
          info_layout->setSpacing(1);
        }else{
          QLabel *message = new QLabel("Unable to find user");
          info_layout->addWidget(message);
        }
        info_box->setLayout(info_layout);
        follow_layout->setAlignment(Qt::AlignLeft);
        follow_layout->setSpacing(2);
        follow_layout->addWidget(info_box);
        follow_box->setLayout(follow_layout);
        follow_box->setStyleSheet("QWidget{background-color: #ddd;}");
        follower_layout->addWidget(follow_box);
      }
    }
  }

  follower_area->setLayout(follower_layout);

  scroll_area->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  scroll_area->setWidget(follower_area);

  username_layout->addWidget(user_name);
  username_layout->addWidget(user_handle);
  username_area->setLayout(username_layout);

  user_layout->addWidget(user_pic);
  user_layout->addWidget(username_area);
  user_layout->setAlignment(Qt::AlignLeft);
  user_layout->setSizeConstraint(QLayout::SetFixedSize);
  user_area->setLayout(user_layout);

  //layout->addWidget(user_pic);
  user_column->addWidget(user_area);
  user_column->addWidget(info_area);


  layout->addLayout(user_column);
  layout->addWidget(scroll_area);
  layout->setAlignment(Qt::AlignTop);
  layout->setSpacing(1);
  parent->setLayout(layout);

  parent->updateGeometry();
  parent->update();

  //cout << follower_area->width() << endl;
  //cout <<  << endl;
  int scroll_width = follower_area->geometry().width();
  scroll_width += scroll_area->style()->pixelMetric(QStyle::PM_ScrollBarExtent);
  scroll_width += scroll_area->style()->pixelMetric(QStyle::PM_MenuPanelWidth);
  parent->setMinimumHeight(500);
  scroll_area->setMinimumWidth(scroll_width);
  scroll_area->setMaximumWidth(scroll_width);
  //cout << parent->width();

}
