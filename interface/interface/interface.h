#pragma once

#ifndef INTERFACE_H
#define INTERFACE_H

#include <QtWidgets/QMainWindow>
#include "ui_interface.h"
#include "./ui_interface.h"
#include "ui_interface.h"
#include "GV.h"
#include <QObject>
#include <QFile>
#include <QAbstractButton>
#include <QPushButton>
#include <QFileDialog>
#include <QProgressDialog>
#include <QDirIterator>
#include <QMenu>
#include <QDir>
#include <QLabel>
#include <QFileInfo>
#include <QGuiApplication>
#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QPixmap> 
#include <QImage> 
#include <QImageReader>
#include <QInputDialog>
#include <QVBoxLayout>
#include <QLayout>
#include <QInputDialog>
#include <QTranslator>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QDialog>
#include <QMouseEvent>
#include <QGraphicsSceneMouseEvent>
#include <QColorDialog>
#include <QPoint>
#include <QKeyEvent>
#include <QProcess>
#include <QRandomGenerator>
#include <QTextEdit>
#include <opencv2/opencv.hpp>
#include <QRegularExpression>
#include <QComboBox>
#include <QTimer>


class interface : public QMainWindow
{
    Q_OBJECT

protected:
    void keyPressEvent(QKeyEvent* event) override;

public:
    interface(QWidget* parent = nullptr);
    ~interface();

    QGraphicsView* graphicsView = nullptr;
    QGraphicsScene* scene = new QGraphicsScene();
    QGraphicsPixmapItem* pixmapItem = nullptr;

    QString folderName = nullptr;
    QString darknetfolder = nullptr;
    QString weights = nullptr;

    QPushButton* selectedButton = nullptr;
    QListWidgetItem* selectedItem = nullptr;

    int buttonCounter = -1;
    int index = 0;
    int currentIndex = 0;
    int listbIndexSelected = 0;
    int imageIndex = 0;
    int dButtonIndex = -1;
    int folderitemquantity = 0;
    int mouseClickCounter = 0;

    //all the variables related to the dinamic buttons
    struct dinamicB
    {
        QColor color;
        QVector<QPointF> firstc;
        QVector<QPointF> secondc;
        QPushButton* button = nullptr;
        QString dirButtons;
        int selectionNr = 0;
        QGraphicsScene* scene;
    };
    std::vector<dinamicB> dinamicButtons;

    std::vector<QString> imagelist;

    bool mousePressEventEnabled = true;
    bool darknetDetect = false;

    QProcess* inferprocess;

private slots:
    void images(std::vector<QString> imagelist);

    void on_info_clicked();
    void nextImage();
    void previousImage();
    void on_button_clicked();
    void on_newFile_clicked();
    void on_addB_clicked();
    void on_editB_clicked();
    void on_undoSelec_clicked();
    void on_train_clicked();
    void on_openFile_clicked();
    void on_redo_clicked();
    void yolov4ComboBox();
    void initializeDarknet();
    void addDynamicButton(const QString& buttonName);
    void addDynamicButtonHelper(const QString& buttonName, const QColor& color);
    void addCoordinatesToFile();
    void runPythonScript(const QString& scriptPath, QString script);
    void runDarknetexe(const QString& scriptPath);
    void on_test_clicked();
    void processDarknetOutput();
    bool alreadySelected(const QString& imageName);
    void yolov4Config();
    void deleteCoordinatesFromFile(int selectionIndex);
    QString duplicatedarknet(QString duplicate);
    void copyImagesToObjFolder(const QString& srcFolder, const QString& dstFolder);
    void objdatafilechange();
    void fileCreate();
    void loadCoords(QGraphicsScene* targetScene);
    void clearEverything();
    bool copyDirRecursively(const QString& srcPath, const QString& dstPath);
    void on_widgetlist_clicked();
    void WidgetListset(const QString& file);
    void mouseClicked(const QPointF& pos);
    void createSelection(QGraphicsScene* targetScene);
    void clearSelections();
    void updateMousePosition(const QPointF& pos);
    void customCursor();

private:
    Ui::interfaceClass ui;
};

#endif

