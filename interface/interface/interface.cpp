#ifndef INTERFACE_CPP
#define INTERFACE_CPP

#include "interface.h"
#include "./ui_interface.h"

interface::interface(QWidget* parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);
    setWindowTitle("MasterChief");

    setFocusPolicy(Qt::StrongFocus);

    ui.info->setEnabled(false);
    ui.widgetlist->setEnabled(false);
    ui.addB->setEnabled(false);
    ui.editB->setEnabled(false);
    ui.undoSelec->setEnabled(false);
    ui.test->setEnabled(false);
    ui.redo->setEnabled(false);

    yolov4ComboBox();

    QObject::connect(ui.openFile, &QAction::triggered, this, &interface::on_openFile_clicked);
    QObject::connect(ui.newFile, &QAction::triggered, this, &interface::on_newFile_clicked);

    ui.graphicsView->setScene(scene);
}

interface::~interface()
{
    clearEverything();
    delete scene;
}
//---------------------- PROJECT MANAGMENT ---------------------------
void interface::on_newFile_clicked() {
    QDialog dialog(this);
    dialog.setWindowTitle("Folder paths");

    QGridLayout* dialogLayout = new QGridLayout(&dialog);

    // Label e botão de seleção do primeiro diretório
    QLabel* dirnameLabel = new QLabel("Image folder path:", &dialog);
    dialogLayout->addWidget(dirnameLabel, 0, 0);

    QPushButton* dirselecB1 = new QPushButton("Select", &dialog);
    dialogLayout->addWidget(dirselecB1, 0, 1);

    QLabel* dirname = new QLabel(&dialog);
    dialogLayout->addWidget(dirname, 1, 0, 1, 2);

    connect(dirselecB1, &QPushButton::clicked, [&]() {
        folderName = QFileDialog::getExistingDirectory(nullptr, "Select an image folder", "", QFileDialog::ShowDirsOnly);
        if (folderName.isEmpty()) {
            ui.label->setText("No folder selected.");
            return;
        }
        dirname->setText(folderName);
        });

    // Label e botão de seleção do segundo diretório
    QLabel* darknetLabel = new QLabel("Darknet copy location:", &dialog);
    dialogLayout->addWidget(darknetLabel, 2, 0);

    QPushButton* dirselecB2 = new QPushButton("Select", &dialog);
    dialogLayout->addWidget(dirselecB2, 2, 1);

    QLabel* darknetpath = new QLabel(&dialog);
    dialogLayout->addWidget(darknetpath, 3, 0, 1, 2);

    connect(dirselecB2, &QPushButton::clicked, [&]() {
        darknetfolder = QFileDialog::getExistingDirectory(nullptr, "Darknet copy location", "", QFileDialog::ShowDirsOnly);
        if (darknetfolder.isEmpty()) {
            ui.label->setText("No folder selected.");
            return;
        }
        darknetfolder = darknetfolder + "/darknet";
        darknetpath->setText(darknetfolder);
        });

    // Botões OK e Cancelar
    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, &dialog);
    dialogLayout->addWidget(buttonBox, 4, 0, 1, 2, Qt::AlignCenter);

    // Conecta os botões OK e Cancelar para aceitar ou rejeitar a janela de diálogo
    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    // Executa a janela de diálogo
    if (dialog.exec() == QDialog::Accepted && !darknetfolder.isEmpty() && !folderName.isEmpty()) {
        QDir currentDir(QDir::currentPath());
        if (!currentDir.cd("..") || !currentDir.cd("..") || !currentDir.cd("..") || !currentDir.cd("masterchief")) {
            ui.label->setText("Failed to locate the masterchief directory.");
            return;
        }
        QString sourceDir = currentDir.absoluteFilePath("darknet");
        QString destDir = darknetfolder;

        qDebug() << "Source Directory:" << sourceDir;
        qDebug() << "Destination Directory:" << destDir;

        if (!copyDirRecursively(sourceDir, destDir)) {
            ui.label->setText("Failed to copy darknet to the specified location.");
            qDebug() << "Failed to copy from" << sourceDir << "to" << destDir;
            return;
        }

        int i = 0;
        qint64 totalSize = 0;

        clearEverything();

        // Desbloquear os botões
        ui.widgetlist->setEnabled(true);
        ui.addB->setEnabled(true);
        ui.editB->setEnabled(true);
        ui.info->setEnabled(true);
        ui.redo->setEnabled(true);

        QFileInfo folderInfo(folderName);
        QDir directory(folderName);

        QStringList files = directory.entryList(QDir::Files);
        folderitemquantity = files.size();
        imagelist.resize(folderitemquantity);

        foreach(const QString & file, files) {
            QString filePath = directory.filePath(file);
            QFileInfo fileInfo(filePath);
            totalSize += fileInfo.size();

            // Detectar formato baseado na extensão do arquivo
            QString extension = fileInfo.suffix().toLower();
            QString format = QImageReader::imageFormat(filePath);

            if ((extension == "png" || extension == "jpg" || extension == "bmp" || extension == "jpeg") && (!format.isEmpty() || extension == "jpg" || extension == "jpeg")) {
                qDebug() << "File:" << filePath << "Format based on extension:" << extension << "Detected format:" << format;
                WidgetListset(file);
                imagelist[i] = filePath;
                i++;
            }
            else {
                qDebug() << "File:" << filePath << "is not a supported format";
            }
        }

        if (imagelist.empty()) {
            ui.label->setText("No valid images found in the folder.");
        }
        else {
            images(imagelist);
        }

        ui.foldername->setText("    Folder name: " + folderInfo.fileName());
        ui.foldersize->setText("    Folder size: " + QString::number(totalSize * 0.000001, 'f', 2) + "mb");
        ui.folderitemquantity->setText("    item quantity: " + QString::number(folderitemquantity));

        copyImagesToObjFolder(folderName, darknetfolder + "/darknet/data/obj");
    }
}

void interface::on_openFile_clicked() {
    QString darknetfilepath = QFileDialog::getExistingDirectory(nullptr, "Select a darknet folder", "", QFileDialog::ShowDirsOnly);
    if (darknetfilepath.isEmpty()) {
        ui.label->setText("No folder selected.");
        return;
    }
    qDebug() << darknetfilepath;

    darknetfolder = darknetfilepath;

    clearEverything();

    QString objFolderPath = darknetfolder + "/data/obj";
    QDir objDir(objFolderPath);
    if (!objDir.exists()) {
        ui.label->setText("The selected folder does not contain the expected structure.");
        return;
    }

    QStringList nameFilters;
    nameFilters << "*.jpg" << "*.jpeg" << "*.png";
    QFileInfoList fileList = objDir.entryInfoList(nameFilters, QDir::Files);

    qint64 totalSize = 0; // Initialize total size

    for (const QFileInfo& fileInfo : fileList) {
        imagelist.push_back(fileInfo.absoluteFilePath());
        WidgetListset(fileInfo.absoluteFilePath());
        totalSize += fileInfo.size(); // Accumulate the file size
    }

    if (imagelist.empty()) {
        ui.label->setText("No images found in the specified folder.");
        return;
    }

    folderitemquantity = imagelist.size();

    QString objNamesFilePath = darknetfolder + "/data/obj.names";
    QFile objNamesFile(objNamesFilePath);
    if (!objNamesFile.exists()) {
        ui.label->setText("The obj.names file does not exist in the expected location.");
        return;
    }

    if (!objNamesFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        ui.label->setText("Failed to open the obj.names file.");
        return;
    }

    QTextStream in(&objNamesFile);
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (!line.isEmpty()) {
            addDynamicButton(line);
        }
    }
    objNamesFile.close();

    if (!imagelist.empty()) {
        imageIndex = 0;
        images({ imagelist[imageIndex] });
        loadCoords(scene);  // Load coordinates into the main scene
        ui.widgetlist->setCurrentRow(imageIndex);
        QFileInfo fileName(imagelist[imageIndex]);
        ui.label->setText(fileName.fileName());
    }

    QFileInfo folderInfo(darknetfilepath);

    ui.foldername->setText("    Folder name: " + folderInfo.fileName());
    ui.foldersize->setText("    Folder size: " + QString::number(totalSize * 0.000001, 'f', 2) + " MB");
    ui.folderitemquantity->setText("    Item quantity: " + QString::number(folderitemquantity));

    ui.widgetlist->setEnabled(true);
    ui.addB->setEnabled(true);
    ui.editB->setEnabled(true);
    ui.redo->setEnabled(true);
    ui.test->setEnabled(true);
    ui.info->setEnabled(true);
    yolov4ComboBox();
}

bool interface::copyDirRecursively(const QString& srcPath, const QString& dstPath) {
    QDir srcDir(srcPath);
    if (!srcDir.exists()) {
        return false;
    }

    QDir dstDir(dstPath);
    if (!dstDir.exists()) {
        if (!dstDir.mkpath(".")) {
            return false;
        }
    }

    foreach(QString file, srcDir.entryList(QDir::Files)) {
        QString srcFilePath = srcPath + "/" + file;
        QString dstFilePath = dstPath + "/" + file;
        if (!QFile::copy(srcFilePath, dstFilePath)) {
            return false;
        }
    }

    foreach(QString dir, srcDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        QString srcDirPath = srcPath + "/" + dir;
        QString dstDirPath = dstPath + "/" + dir;
        if (!copyDirRecursively(srcDirPath, dstDirPath)) {
            return false;
        }
    }

    return true;
}

//---------------------- BUTTONS MANAGMENT---------------------------
void interface::on_addB_clicked() {
    QColor color = Qt::black; // Define a cor preta como padrão

    // Cria uma nova janela de diálogo
    QDialog dialog(this);
    dialog.setWindowTitle("New button");

    // Layout para a janela de diálogo
    QVBoxLayout* dialogLayout = new QVBoxLayout(&dialog);

    // Info label
    QLabel* infolabel = new QLabel("Choose a button name and color", &dialog);
    dialogLayout->addWidget(infolabel);

    // LineEdit para o nome do botão
    QLineEdit* objectnameB = new QLineEdit(&dialog);
    objectnameB->setPlaceholderText("Button name");
    dialogLayout->addWidget(objectnameB);

    // Define o foco no LineEdit
    objectnameB->setFocus();

    QLabel* infolabel1 = new QLabel("\nChoose a color for the selection", &dialog);
    dialogLayout->addWidget(infolabel1);

    QHBoxLayout* colorLayout = new QHBoxLayout();
    dialogLayout->addLayout(colorLayout);

    QPushButton* colorButton = new QPushButton("Color", &dialog);
    colorLayout->addWidget(colorButton);

    QLabel* colorDisplay = new QLabel(&dialog);
    colorDisplay->setStyleSheet("background-color: black; border: 1px solid black;");
    colorLayout->addWidget(colorDisplay);

    connect(colorButton, &QPushButton::clicked, [&]() {
        QColor selectedColor = QColorDialog::getColor();
        if (selectedColor.isValid()) {
            color = selectedColor;
            colorDisplay->setStyleSheet(QString("background-color: %1; border: 1px solid black;").arg(color.name()));
        }
        // Se nenhuma cor foi escolhida, a cor padrão permanece preta
        });

    QLabel* infolabel2 = new QLabel("To stop the selection, click again on the created button", &dialog);
    dialogLayout->addWidget(infolabel2);

    // Botões OK e Cancelar
    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, &dialog);
    dialogLayout->addWidget(buttonBox, 0, Qt::AlignHCenter);

    // Conecta os botões OK e Cancelar para aceitar ou rejeitar a janela de diálogo
    QObject::connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    // Ajusta o tamanho do diálogo ao conteúdo
    dialog.adjustSize();

    // Executa a janela de diálogo
    if (dialog.exec() == QDialog::Accepted) {
        QString objectnameButton = objectnameB->text();
        if (!objectnameButton.isEmpty()) {
            addDynamicButtonHelper(objectnameButton, color);
        }
    }
}

void interface::on_editB_clicked() {
    // create a dialog window
    QDialog dialog(this);
    dialog.setWindowTitle("Button list");

    // main layout 
    QVBoxLayout* vdialogLayout = new QVBoxLayout(&dialog);

    // list of buttons
    QListWidget* listB = new QListWidget(&dialog);
    vdialogLayout->addWidget(listB);

    // obtains the buttons for the layout
    QLayout* layout = ui.objetos->layout();
    for (int i = 0; i < layout->count(); ++i) {
        // verification
        if (QPushButton* button = qobject_cast<QPushButton*>(layout->itemAt(i)->widget())) {
            // button add
            QListWidgetItem* item = new QListWidgetItem(button->text(), listB);
            item->setData(Qt::UserRole, QVariant::fromValue(button));
        }
    }

    // second layout for organization
    QHBoxLayout* hdialogLayout = new QHBoxLayout(&dialog);

    // delete button
    QPushButton* deleteB = new QPushButton("Delete", &dialog);
    hdialogLayout->addWidget(deleteB);

    // edit button
    QPushButton* editB = new QPushButton("Edit", &dialog);
    hdialogLayout->addWidget(editB);

    // add the vertical layout to the horizontal one
    vdialogLayout->addLayout(hdialogLayout);

    // connects the click on the list to the actual button
    connect(listB, &QListWidget::itemClicked, this, [&](QListWidgetItem* item) {
        // verification
        if (item) {
            // obtains the button associated to the item
            selectedButton = item->data(Qt::UserRole).value<QPushButton*>();
            selectedItem = item;
            listbIndexSelected = listB->row(item);
        }
        });

    // edit button 
    connect(editB, &QPushButton::clicked, [&]() {
        if (selectedButton) {
            // ask for the new name
            QString oldobjectname = selectedButton->text();
            bool ok;
            QString newName = QInputDialog::getText(&dialog, tr("Edit button"),
                tr("Choose the new button name"),
                QLineEdit::Normal,
                selectedButton->text(),
                &ok);

            // verification and update to the list
            if (ok && !newName.isEmpty()) {
                // update the name on the storage array and text variables
                dinamicButtons[listbIndexSelected].button->setText(newName);
                selectedButton->setText(dinamicButtons[listbIndexSelected].button->text());
                selectedItem->setText(dinamicButtons[listbIndexSelected].button->text());

                // rename
                QDir directory(dinamicButtons[listbIndexSelected].dirButtons);
                QString oldPath = QDir(dinamicButtons[listbIndexSelected].dirButtons).filePath(oldobjectname);
                QString newPath = QDir(dinamicButtons[listbIndexSelected].dirButtons).filePath(dinamicButtons[listbIndexSelected].button->text());
                dinamicButtons[listbIndexSelected].dirButtons = newPath;
                dinamicButtons[listbIndexSelected].button->setObjectName(QString(newName));
                if (directory.rename(oldPath, newPath)) {
                    ui.label->setText("Folder renamed successfully");
                }
                else {
                    ui.label->setText("Error renaming the folder");
                }
            }
            else {
                ui.label->setText("Name not valid");
            }
        }
        else {
            ui.label->setText("Selection button does not exist");
        }
        });

    // delete button
    connect(deleteB, &QPushButton::clicked, [&]() {
        if (selectedButton && selectedItem) {
            // Cria o caminho
            QString path = QDir(dinamicButtons[listbIndexSelected].dirButtons).filePath(dinamicButtons[listbIndexSelected].button->text());
            QDir directory(path);

                    // Remove o item da lista
            listB->takeItem(listB->row(selectedItem));

            // Remove do vetor de armazenamento
            dinamicButtons.erase(dinamicButtons.begin() + listbIndexSelected);
            delete selectedButton;

            // size vector
            dButtonIndex--;
            dinamicButtons.resize(dButtonIndex + 1);

            objdatafilechange();//updates the obj.data in darknet folder

            // Limpa a seleção
            selectedButton = nullptr;
            selectedItem = nullptr;
            listbIndexSelected = -1;
        }
        else {
            ui.label->setText("Selection is null or no button is selected");
        }
        });

    dialog.exec();
}

void interface::on_info_clicked() {
    QDialog dialog(this);
    dialog.setWindowTitle("Macros info");

    QVBoxLayout* vdialogLayout = new QVBoxLayout(&dialog);
    QLabel* label1 = new QLabel(&dialog);
    label1->setText("A - Previous Image \nD - Next Image\n1 - First Class Button\n2 - Second Class Button\n3 - Third Class Button\nT - Enable Test");
    vdialogLayout->addWidget(label1);

    dialog.exec();
}

void interface::on_button_clicked() {
    QPushButton* clickedButton = qobject_cast<QPushButton*>(sender());
    if (clickedButton) {
        ui.label->setText("Button clicked: " + clickedButton->text());
        qDebug() << "Button clicked: " << clickedButton->text();

        int buttonIndex = -1;
        for (int i = 0; i < dinamicButtons.size(); ++i) {
            if (dinamicButtons[i].button == clickedButton) {
                buttonIndex = i;
                break;
            }
        }

        if (buttonIndex != -1) {
            qDebug() << "Button index: " << buttonIndex;
            if (mousePressEventEnabled) {
                // Inicializa a seleção
                mouseClickCounter = 0;
                dButtonIndex = buttonIndex;
                customCursor();

                // Desabilita outros botões
                for (int i = 0; i < dinamicButtons.size(); ++i) {
                    if (i != buttonIndex) {
                        dinamicButtons[i].button->setEnabled(false);
                    }
                }

                // Desabilita outros controles
                ui.info->setEnabled(false);
                ui.widgetlist->setEnabled(false);
                ui.addB->setEnabled(false);
                ui.editB->setEnabled(false);
                ui.undoSelec->setEnabled(true);

                // Habilita o estado de captura de mouse
                mousePressEventEnabled = false;
            }
            else {
                // Finaliza a seleção
                mousePressEventEnabled = true;
                ui.graphicsView->viewport()->unsetCursor();

                // Salva as seleções no arquivo
                fileCreate();

                // Reabilita todos os botões
                for (int i = 0; i < dinamicButtons.size(); ++i) {
                    dinamicButtons[i].button->setEnabled(true);
                }

                // Reabilita outros controles
                ui.info->setEnabled(true);
                ui.widgetlist->setEnabled(true);
                ui.addB->setEnabled(true);
                ui.editB->setEnabled(true);
                ui.undoSelec->setEnabled(false);

                // Carrega e redesenha todas as seleções na cena principal
                loadCoords(scene);  // Use the main scene
            }
        }
        else {
            ui.label->setText("Button not found in dinamicButtons array.");
            qDebug() << "Button not found in dinamicButtons array.";
        }
    }
}

void interface::addDynamicButton(const QString& buttonName) {
    static int hueOffset = 240;  // Start with a hue offset of 0 (which corresponds to red)

    // HSV color model: Start with red (hue 0), full saturation, and full brightness
    QColor borderColor = QColor::fromHsv(hueOffset, 255, 255);

    // Add the dynamic button with the calculated color
    addDynamicButtonHelper(buttonName, borderColor);

    // Increment the hueOffset for the next button, looping within the hue range [0, 360)
    const int hueIncrement = 60;  // Change this value to control the color shift between buttons
    hueOffset = (hueOffset + hueIncrement) % 360;
}

void interface::addDynamicButtonHelper(const QString& buttonName, const QColor& color) {
    dButtonIndex++;
    dinamicButtons.resize(dButtonIndex + 1); // Resize button vector
    dinamicButtons[dButtonIndex].color = color;

    // Cria o botão com o nome predefinido
    QPushButton* newButton = new QPushButton(buttonName);
    dinamicButtons[dButtonIndex].button = newButton;
    dinamicButtons[dButtonIndex].button->setObjectName(buttonName);

    // Atualiza o arquivo obj.data
    objdatafilechange();

    // Define o estilo do botão com a cor especificada
    QString buttonStyle = QString("border: 2px solid %1;").arg(color.name());
    dinamicButtons[dButtonIndex].button->setStyleSheet(buttonStyle);

    // Adiciona o botão ao layout
    QVBoxLayout* layout = qobject_cast<QVBoxLayout*>(ui.objetos->layout());
    if (layout) {
        int spacerIndex = layout->indexOf(ui.verticalSpacer);
        layout->insertWidget(spacerIndex, newButton);
    }

    // Conecta o botão ao slot on_button_clicked
    QObject::connect(dinamicButtons[dButtonIndex].button, &QPushButton::clicked, this, &interface::on_button_clicked);
}

void interface::keyPressEvent(QKeyEvent* event) {
    switch (event->key()) {
    case Qt::Key_T:
        // Always active
        on_test_clicked();  // Call the on_test_clicked() function
        break;

    case Qt::Key_1:
        // Always active
        if (!dinamicButtons.empty()) {
            QPushButton* firstButton = dinamicButtons[0].button;
            if (firstButton) {
                firstButton->click();  // Simulate a click on the first dynamic button
            }
        }
        break;

    case Qt::Key_2:
        // Always active
        if (dinamicButtons.size() > 1) {
            QPushButton* secondButton = dinamicButtons[1].button;
            if (secondButton) {
                secondButton->click();  // Simulate a click on the second dynamic button
            }
        }
        break;

    case Qt::Key_3:
        // Always active
        if (dinamicButtons.size() > 2) {
            QPushButton* thirdButton = dinamicButtons[2].button;
            if (thirdButton) {
                thirdButton->click();  // Simulate a click on the third dynamic button
            }
        }
        break;

    case Qt::Key_A:
        // Active only when mousePressEventEnabled is true
        if (mousePressEventEnabled) {
            previousImage();
        }
        else {
            QWidget::keyPressEvent(event);  // Default handling for the "A" key
        }
        break;

    case Qt::Key_D:
        // Active only when mousePressEventEnabled is true
        if (mousePressEventEnabled) {
            nextImage();
        }
        else {
            QWidget::keyPressEvent(event);  // Default handling for the "D" key
        }
        break;

    default:
        QWidget::keyPressEvent(event);  // Default handling for all other keys
        break;
    }
}

//------------------ IMAGES MANAGMENT---------------------
void interface::copyImagesToObjFolder(const QString& sourceFolder, const QString& destinationFolder) {
    QString sourcfol = sourceFolder;
    QString destfol = destinationFolder;
    
    destfol = duplicatedarknet(destfol);

    QDir sourceDir(sourceFolder);
    QDir destDir(destinationFolder);

    if (!destDir.exists()) {
        if (!destDir.mkpath(".")) {
            qDebug() << "Failed to create destination directory:" << destinationFolder;
            return;
        }
    }

    QStringList imageFiles = sourceDir.entryList(QDir::Files);

    foreach(const QString & imageFile, imageFiles) {
        QString sourceFilePath = sourceDir.absoluteFilePath(imageFile);
        QString destFilePath = destDir.absoluteFilePath(imageFile);
        destFilePath = duplicatedarknet(destFilePath);

        if (QFile::exists(destFilePath)) {
            QFile::remove(destFilePath);
        }

        if (QFile::copy(sourceFilePath, destFilePath)) {
            qDebug() << "Copied" << sourceFilePath << "to" << destFilePath;
        }
        else {
            qDebug() << "Failed to copy" << sourceFilePath << "to" << destFilePath;
        }
    }
}

void interface::previousImage() {
    scene->clear();
    if (imageIndex > 0) {
        imageIndex--;
    }
    else {
        imageIndex = imagelist.size() - 1;
    }
    images({ imagelist[imageIndex] });
    ui.widgetlist->setCurrentRow(imageIndex);
    QFileInfo fileName(imagelist[imageIndex]);
    ui.label->setText(fileName.fileName());
    clearSelections();
    loadCoords(scene);  // Pass the main scene to loadCoords
}

void interface::nextImage() {
    scene->clear();
    if (imageIndex < imagelist.size() - 1) {
        imageIndex++;
    }
    else {
        imageIndex = 0;
    }
    images({ imagelist[imageIndex] });
    ui.widgetlist->setCurrentRow(imageIndex);
    QFileInfo fileName(imagelist[imageIndex]);
    ui.label->setText(fileName.fileName());
    clearSelections();
    loadCoords(scene);  // Pass the main scene to loadCoords
    if (darknetDetect) {
        bool selectionVerification = alreadySelected(imagelist[imageIndex]);
        qDebug() << "Selection verification:" << selectionVerification;
        if (selectionVerification == false) {
            qDebug() << "DARKNET SELECTION ACTIVATED";

            if (inferprocess && inferprocess->state() == QProcess::Running) {
                QString imageFileName = imagelist[imageIndex];
                qDebug() << "Sending image path to Darknet process:" << imageFileName;

                // Envie o caminho da imagem para o processo do Darknet
                inferprocess->write(imageFileName.toUtf8() + '\n');
            }
            else {
                qDebug() << "Darknet process is not running.";
            }
        }
    }
}

bool interface::alreadySelected(const QString& imageName) {
    // Cria um QFileInfo para a imagem com caminho absoluto
    QFileInfo imageFile(imageName);

    qDebug() << "Verificando existência da imagem:" << imageName;

    // Verifica se a imagem existe
    if (!imageFile.exists()) {
        qDebug() << "Erro: A imagem" << imageFile.filePath() << "não existe.";
        return false;
    }

    // Extrai o diretório da imagem
    QString directoryPath = imageFile.absolutePath();

    // Cria o caminho do arquivo de texto correspondente
    QString txtFilePath = directoryPath + "/" + imageFile.completeBaseName() + ".txt";
    QFileInfo txtFile(txtFilePath);

    qDebug() << "Verificando existência do arquivo de texto:" << txtFilePath;

    if (txtFile.exists()) {
        qDebug() << "File already has selections";
        return true;
    }
    else {
        qDebug() << "File doesn't have selections";
        return false;
    }
}

void interface::on_widgetlist_clicked() {
    QListWidgetItem* selectedItem = ui.widgetlist->currentItem();
    if (!selectedItem) {
        ui.label->setText("No image selected.");
        return;
    }

    int newIndex = ui.widgetlist->row(selectedItem);
    if (newIndex < 0 || newIndex >= imagelist.size()) {
        ui.label->setText("Failed to load the image: Index out of range.");
        return;
    }

    imageIndex = newIndex;  // Update the current index

    // Clear selections if there's an active dynamic button with valid coordinates
    if (dButtonIndex != -1 && !dinamicButtons[dButtonIndex].firstc.isEmpty() && !dinamicButtons[dButtonIndex].secondc.isEmpty()) {
        clearSelections();
    }

    // Load the selected image and its coordinates
    images({ imagelist[imageIndex] });
    loadCoords(scene);

    // Update the label with the image's file name
    QFileInfo fileInfo(imagelist[imageIndex]);
    ui.label->setText(fileInfo.fileName());

    // Reset focus to the main widget to enable macro capture again
    this->setFocus();
}

void interface::WidgetListset(const QString& file) {
    QFileInfo fileInfo(file);
    ui.widgetlist->addItem(fileInfo.fileName());
}

void interface::images(std::vector<QString> imagelist) {
    scene->clear();
    if (imagelist.empty()) {
        ui.label->setText("Folder list empty.");
        return;
    }

    QString imagePath = imagelist[0];
    qDebug() << "Attempting to load image:" << imagePath;

    QImageReader reader(imagePath);
    if (!reader.canRead()) {
        ui.label->setText(QString("Cannot read the image: %1").arg(imagePath));
        qDebug() << "Cannot read the image:" << imagePath << "Error:" << reader.errorString();
        return;
    }

    QImage image = reader.read();
    if (image.isNull()) {
        ui.label->setText(QString("Failed to load the image: %1").arg(imagePath));
        qDebug() << "Failed to load the image:" << imagePath << "Error:" << reader.errorString();
        return;
    }

    QPixmap pixmap = QPixmap::fromImage(image);
    if (pixmap.isNull()) {
        ui.label->setText(QString("Failed to convert the image: %1").arg(imagePath));
        qDebug() << "Failed to convert the image:" << imagePath;
        return;
    }

    QGraphicsPixmapItem* pixmapItem = new QGraphicsPixmapItem(pixmap);
    scene->addItem(pixmapItem);

    // Ajusta a cena ao tamanho da imagem
    scene->setSceneRect(pixmap.rect());

    // Ajusta o QGraphicsView para escalar a cena
    ui.graphicsView->fitInView(scene->sceneRect(), Qt::KeepAspectRatio);

    // Conecta os eventos do mouse
    GV* graphicsView = qobject_cast<GV*>(ui.graphicsView);
    if (graphicsView) {
        QObject::disconnect(graphicsView, &GV::mouseMoved, this, &interface::updateMousePosition);
        QObject::disconnect(graphicsView, &GV::mousePressed, this, &interface::mouseClicked);

        QObject::connect(graphicsView, &GV::mouseMoved, this, &interface::updateMousePosition);
        QObject::connect(graphicsView, &GV::mousePressed, this, &interface::mouseClicked);
    }
    else {
        ui.label->setText("Failed to cast graphicsView to GV");
        qDebug() << "Failed to cast graphicsView to GV";
    }
}

//------------------------ MOUSE EVENTS MANAGEMENT---------------------------
void interface::customCursor() {
    QPixmap cursorPixmap(600, 600);
    cursorPixmap.fill(Qt::transparent);
    QPainter painter(&cursorPixmap);
    painter.setPen(Qt::black);
    painter.drawLine(300, 0, 300, 600);
    painter.drawLine(0, 300, 600, 300);
    painter.end();
    QCursor customCursor(cursorPixmap, 300, 300);
    ui.graphicsView->viewport()->setCursor(customCursor);

    qDebug() << "Custom cursor set.";
}

void interface::updateMousePosition(const QPointF& pos) {
    ui.label2->setText(QString("x: %1 y: %2)").arg(pos.x()).arg(pos.y()));
}

void interface::mouseClicked(const QPointF& pos) {
    if (!mousePressEventEnabled) {
        qDebug() << "Mouse clicked at: " << pos;

        if (mouseClickCounter == 0) {
            dinamicButtons[dButtonIndex].firstc.push_back(pos);
            mouseClickCounter++;
            qDebug() << "First point selected: " << pos;
        }
        else if (mouseClickCounter == 1) {
            dinamicButtons[dButtonIndex].secondc.push_back(pos);
            mouseClickCounter = 0;
            dinamicButtons[dButtonIndex].selectionNr++;
            ui.selectionNr->setText(QString("Number of selections: %1").arg(dinamicButtons[dButtonIndex].selectionNr));
            qDebug() << "Second point selected: " << pos;
            createSelection(scene);  // Use the main scene
        }
        else {
            ui.label->setText("Selection error");
            qDebug() << "Selection error: Unexpected mouseClickCounter value.";
        }
    }
    else {
        qDebug() << "Mouse click ignored: mousePressEventEnabled is false.";
    }
}

//---------------------------------------------Selection Management----------------------
void interface::deleteCoordinatesFromFile(int selectionIndex) {
    // Construct the path to the coordinates file
    QString imagePath = imagelist[imageIndex];
    QFileInfo imageFileInfo(imagePath);
    QString baseName = imageFileInfo.completeBaseName();
    QString coordsFilePath = darknetfolder + "/darknet/data/obj/" + baseName + ".txt";
    coordsFilePath= duplicatedarknet(coordsFilePath);

    qDebug() << "Coordinates file path:" << coordsFilePath;

    QFile coordsFile(coordsFilePath);
    if (!coordsFile.exists()) {
        qDebug() << "Coordinates file does not exist.";
        return;
    }

    if (!coordsFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Failed to open coordinates file for reading.";
        return;
    }

    // Read all lines from the file
    QTextStream in(&coordsFile);
    QStringList lines;
    while (!in.atEnd()) {
        lines.append(in.readLine());
    }
    coordsFile.close();

    // Check if the selection index is within bounds
    if (selectionIndex < 0 || selectionIndex >= lines.size()) {
        qDebug() << "Invalid selection index. Cannot delete.";
        return;
    }

    // Remove the line corresponding to the selectionIndex
    qDebug() << "Deleting line:" << lines[selectionIndex];
    lines.removeAt(selectionIndex);

    // Write the modified lines back to the file
    if (!coordsFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        qDebug() << "Failed to open coordinates file for writing.";
        return;
    }
    QTextStream out(&coordsFile);
    for (const QString& line : lines) {
        out << line << "\n";
    }
    coordsFile.close();

    qDebug() << "Coordinates file updated successfully.";
}

void interface::loadCoords(QGraphicsScene* targetScene) {
    targetScene->clear();
    for (auto& button : dinamicButtons) {
        button.firstc.clear();
        button.secondc.clear();
    }

    QString imagePath = imagelist[imageIndex];
    QFileInfo imageFileInfo(imagePath);
    QString baseName = imageFileInfo.completeBaseName();

    QString objFolderPath = darknetfolder + "/darknet/data/obj";
    objFolderPath = duplicatedarknet(objFolderPath); // remove the possible /darknet/darknet
    QString coordsFilePath = objFolderPath + "/" + baseName + ".txt";

    QFile coordsFile(coordsFilePath);
    if (!coordsFile.exists()) {
        qDebug() << "File does not exist:" << coordsFilePath;
    }

    if (!coordsFile.open(QIODevice::ReadOnly)) {
        qDebug() << "Failed to open file:" << coordsFilePath;
    }

    QTextStream in(&coordsFile);
    while (!in.atEnd()) {
        QString line = in.readLine();
        if (line.isEmpty()) {
            qDebug() << "Empty line in file:" << coordsFilePath;
            continue;
        }

        QStringList coordinates = line.split(' ', Qt::SkipEmptyParts);

        if (coordinates.size() % 5 != 0) {
            qDebug() << "Invalid number of coordinate values in line:" << line;
            continue;
        }

        int classIndex = coordinates[0].toInt();
        for (int j = 0; j < coordinates.size(); j += 7) {
            bool ok1, ok2, ok3, ok4;
            float cx = coordinates[j + 1].toFloat(&ok1);
            float cy = coordinates[j + 2].toFloat(&ok2);
            float width = coordinates[j + 3].toFloat(&ok3);
            float height = coordinates[j + 4].toFloat(&ok4);

            float x1 = cx - width / 2;
            float y1 = cy - height / 2;
            float x2 = x1 + width;
            float y2 = y1 + height;

            if (ok1 && ok2 && ok3 && ok4) {
                QPointF first(x1, y1);
                QPointF second(x2, y2);
                dinamicButtons[classIndex].firstc.push_back(first);
                dinamicButtons[classIndex].secondc.push_back(second);
            }
            else {
                qDebug() << "Invalid coordinate values in line:" << line;
            }
            qDebug() << "Coordinate values:" << line;
        }
    }

    coordsFile.close();
    createSelection(targetScene);
}

void interface::createSelection(QGraphicsScene* targetScene) {
    targetScene->clear();  // Clear the scene before adding new selections

    QPixmap pixmap(imagelist[imageIndex]);
    QGraphicsPixmapItem* pixmapItem = new QGraphicsPixmapItem(pixmap);
    targetScene->addItem(pixmapItem);

    int imgWidth = pixmap.width();
    int imgHeight = pixmap.height();

    for (int i = 0; i < dinamicButtons.size(); ++i) {
        for (int j = 0; j < dinamicButtons[i].firstc.size(); ++j) {
            QPointF first = dinamicButtons[i].firstc[j];
            QPointF second = dinamicButtons[i].secondc[j];

            qreal x1 = first.x() * imgWidth;
            qreal y1 = first.y() * imgHeight;
            qreal x2 = second.x() * imgWidth;
            qreal y2 = second.y() * imgHeight;

            QRectF rect(QPointF(x1, y1), QPointF(x2, y2));
            QRectF normalizedRect = rect.normalized();

            QGraphicsRectItem* rectItem = new QGraphicsRectItem(normalizedRect);

            QPen pen(dinamicButtons[i].color);
            pen.setWidth(2);

            rectItem->setPen(pen);
            targetScene->addItem(rectItem);
        }
    }

    qDebug() << "Selections created.";
}

void interface::on_undoSelec_clicked() {
    if (dButtonIndex >= 0 && dButtonIndex < dinamicButtons.size()) {
        // Verifica se há seleções para desfazer
        if (!dinamicButtons[dButtonIndex].firstc.isEmpty() && !dinamicButtons[dButtonIndex].secondc.isEmpty()) {
            // Remove a última seleção feita
            dinamicButtons[dButtonIndex].firstc.pop_back();
            dinamicButtons[dButtonIndex].secondc.pop_back();

            // Recria as seleções restantes na cena principal
            createSelection(scene);  // Use the main scene

            // Atualiza o número de seleções
            dinamicButtons[dButtonIndex].selectionNr = dinamicButtons[dButtonIndex].firstc.size();
            ui.selectionNr->setText("Number of selections: " + QString::number(dinamicButtons[dButtonIndex].selectionNr));

            // Enable the Undo button if there are still selections to undo
            ui.undoSelec->setEnabled(!dinamicButtons[dButtonIndex].firstc.isEmpty());
        }
        else {
            ui.label->setText("No selection to undo.");
            // Disable Undo button since there's nothing to undo
            ui.undoSelec->setEnabled(false);
        }
    }
    else {
        ui.label->setText("No dynamic button selected.");
        // Disable Undo button since there's nothing to undo
        ui.undoSelec->setEnabled(false);
    }
}

void interface::clearSelections() {
    for (auto& button : dinamicButtons) {
        button.firstc.clear();
        button.secondc.clear();
        button.selectionNr = 0;
    }
    qDebug() << "Selections cleared.";
}

void interface::clearEverything() {
    ui.widgetlist->clear();

    if (!dinamicButtons.empty()) {
        for (auto& item : dinamicButtons) {
            if (item.button) {
                delete item.button;
                item.button = nullptr;
            }
            item.firstc.clear();
            item.secondc.clear();
            item.selectionNr = 0;

            if (item.scene) {
                delete item.scene;
                item.scene = nullptr;
            }
        }
        dinamicButtons.clear();
    }
    imagelist.clear();
    imageIndex = 0;
    folderitemquantity = 0;
    dButtonIndex = -1;
    scene->clear();
}

void interface::on_redo_clicked() {
    // Create and configure the dialog
    QDialog* reviewDialog = new QDialog(this);
    reviewDialog->setWindowTitle("Review Selections");
    reviewDialog->resize(800, 600);

    QVBoxLayout* layout = new QVBoxLayout(reviewDialog);

    // Create the scene and view for the dialog
    QGraphicsScene* reviewScene = new QGraphicsScene();
    QGraphicsView* graphicsView = new QGraphicsView(reviewDialog);
    graphicsView->setScene(reviewScene);
    graphicsView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    layout->addWidget(graphicsView);

    // Display the image and selections
    images({ imagelist[imageIndex] });  // Set up the image in the scene
    loadCoords(reviewScene);            // Load the coordinates and draw the selections in the dialog's scene

    // Track the current selection
    int currentSelectionIndex = -1;
    QList<QGraphicsRectItem*> rectItems;

    // Populate rectItems with the rectangles created in loadCoords, with the last item being first
    QList<QGraphicsRectItem*> tempItems;
    for (QGraphicsItem* item : reviewScene->items()) {
        if (QGraphicsRectItem* rectItem = qgraphicsitem_cast<QGraphicsRectItem*>(item)) {
            tempItems.append(rectItem);
        }
    }

    // Reverse the order
    for (int i = tempItems.size() - 1; i >= 0; --i) {
        rectItems.append(tempItems[i]);
    }

    qDebug() << "Initial number of selections:" << rectItems.size();

    // Button to cycle through selections
    QPushButton* cycleButton = new QPushButton("Next Selection", reviewDialog);
    layout->addWidget(cycleButton);

    // Button to redo (delete) the highlighted selection
    QPushButton* deleteButton = new QPushButton("Delete Selection", reviewDialog);
    layout->addWidget(deleteButton);
    deleteButton->setEnabled(false);  // Initially disabled until a selection is highlighted

    // Connect the cycle button to cycle through selections
    connect(cycleButton, &QPushButton::clicked, this, [&]() mutable {
        if (!rectItems.isEmpty()) {
            // Reset previous selection's color
            if (currentSelectionIndex >= 0) {
                QPen originalPen(dinamicButtons[dButtonIndex].color);
                originalPen.setWidth(2);
                rectItems[currentSelectionIndex]->setPen(originalPen);
            }

            // Move to the next selection
            currentSelectionIndex = (currentSelectionIndex + 1) % rectItems.size();
            qDebug() << "Highlighting selection at index:" << currentSelectionIndex;

            // Highlight the current selection
            QPen highlightPen(Qt::green);  // Use a contrasting color (e.g., green)
            highlightPen.setWidth(3);        // Make the line slightly thicker
            rectItems[currentSelectionIndex]->setPen(highlightPen);

            // Enable the redo button now that a selection is highlighted
            deleteButton->setEnabled(true);
        }
        });

    // Connect the redo button to delete the highlighted selection
    connect(deleteButton, &QPushButton::clicked, this, [&]() mutable {
        qDebug() << "Redo button clicked";
        qDebug() << "Current selection index:" << currentSelectionIndex;
        qDebug() << "Total selections before deletion:" << rectItems.size();

        if (currentSelectionIndex >= 0 && currentSelectionIndex < rectItems.size()) {
            // Remove the selected rectangle from the scene
            QGraphicsRectItem* rectItem = rectItems.takeAt(currentSelectionIndex);
            qDebug() << "Deleting rectangle at index:" << currentSelectionIndex;
            reviewScene->removeItem(rectItem);
            delete rectItem;

            // Remove the corresponding selection from dinamicButtons
            dinamicButtons[dButtonIndex].firstc.remove(currentSelectionIndex);
            dinamicButtons[dButtonIndex].secondc.remove(currentSelectionIndex);

            // Delete the corresponding line from the coordinates file
            deleteCoordinatesFromFile(currentSelectionIndex);

            // Update selectionNr correctly
            dinamicButtons[dButtonIndex].selectionNr = rectItems.size();
            qDebug() << "Selection removed. Total remaining:" << dinamicButtons[dButtonIndex].selectionNr;

            // Adjust the current selection index
            if (rectItems.isEmpty()) {
                // No selections left
                currentSelectionIndex = -1;
                deleteButton->setEnabled(false);
            }
            else {
                // Ensure the index wraps correctly if needed
                currentSelectionIndex = std::min(currentSelectionIndex, static_cast<int>(rectItems.size()) - 1);

                QPen highlightPen(Qt::green);
                highlightPen.setWidth(3);
                rectItems[currentSelectionIndex]->setPen(highlightPen);
                qDebug() << "Next selection highlighted at index:" << currentSelectionIndex;
            }

            // Update the UI
            ui.selectionNr->setText("Number of selections: " + QString::number(dinamicButtons[dButtonIndex].selectionNr));
        }
        else {
            qDebug() << "Invalid selection index for deletion";
        }
        });

    // Show the dialog and wait for it to close
    reviewDialog->exec();
    delete reviewDialog;

    // After the dialog closes, refresh the main view
    scene->clear();                    // Clear the main scene
    loadCoords(scene);                 // Reload the updated coordinates
    createSelection(scene);            // Recreate selections in the main scene
}

void interface::addCoordinatesToFile() {
    // Path to the coordinates file
    QString coordinatesFilePath = darknetfolder + "/darknet/data/coordinates.txt";
    coordinatesFilePath = duplicatedarknet(coordinatesFilePath);
    QFile coordinatesFile(coordinatesFilePath);

    if (!coordinatesFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Failed to open coordinates file for reading:" << coordinatesFile.errorString()<< coordinatesFilePath;
        return;
    }

    QTextStream in(&coordinatesFile);
    QStringList coordinatesList;

    while (!in.atEnd()) {
        QString line = in.readLine();
        coordinatesList.append(line);
    }

    coordinatesFile.close();

    if (coordinatesList.isEmpty()) {
        qDebug() << "Coordinates file is empty.";
        return;
    }

    // Path of the current image
    QString imagePath = imagelist[imageIndex];
    QFileInfo imageFileInfo(imagePath);
    QString baseName = imageFileInfo.completeBaseName();
    QImage image(imagePath);

    // Get the dimensions of the image
    qreal imgWidth = image.width();
    qreal imgHeight = image.height();

    // Path to the obj folder
    QString objFolderPath = darknetfolder + "/darknet/data/obj";
    objFolderPath = duplicatedarknet(objFolderPath);

    QString newImagePath = objFolderPath + "/" + imageFileInfo.fileName();
    QFile::copy(imagePath, newImagePath);

    // Path to the corresponding .txt file for the image
    QString cfolderPath = objFolderPath + "/" + baseName + ".txt";
    QFile txtFile(cfolderPath);

    if (!txtFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qDebug() << "Failed to open target file for writing:" << txtFile.errorString();
        return;
    }

    QTextStream out(&txtFile);

    for (const QString& line : coordinatesList) {
        QStringList coordinates = line.split(" ");
        if (coordinates.size() != 5) {
            qDebug() << "Invalid coordinates format in line:" << line;
            continue;
        }

        // Extract object index (class index) and coordinates
        int classIndex = coordinates[0].toInt();
        qreal x = coordinates[1].toDouble();
        qreal y = coordinates[2].toDouble();
        qreal width = coordinates[3].toDouble();
        qreal height = coordinates[4].toDouble();

        // Convert to center format (centerX, centerY)
        qreal centerX = x + width / 2.0;
        qreal centerY = y + height / 2.0;

        // Normalize with respect to image size
        double normalizedCenterX = centerX / imgWidth;
        double normalizedCenterY = centerY / imgHeight;
        double normalizedWidth = width / imgWidth;
        double normalizedHeight = height / imgHeight;

        // Write the class index and normalized coordinates to the output file
        out << QString::number(classIndex) << " "
            << normalizedCenterX << " " << normalizedCenterY << " "
            << normalizedWidth << " " << normalizedHeight << "\n";

        // Debug: Display normalized coordinates
        qDebug() << "Original (Real): X:" << x << "Y:" << y
            << "Width:" << width << "Height:" << height;
        qDebug() << "Converted to Center (Real): CenterX:" << centerX << "CenterY:" << centerY;
        qDebug() << "Normalized (0-1): CenterX:" << normalizedCenterX
            << "CenterY:" << normalizedCenterY
            << "Width:" << normalizedWidth << "Height:" << normalizedHeight;
    }

    txtFile.close();

    qDebug() << "Coordinates added to file:" << cfolderPath;
}

void interface::fileCreate() {
    QString imagePath = imagelist[imageIndex];
    QFileInfo imageFileInfo(imagePath);
    QString baseName = imageFileInfo.completeBaseName();

    QString objFolderPath = darknetfolder + "/darknet/data/obj";
    objFolderPath = duplicatedarknet(objFolderPath);

    QString newImagePath = objFolderPath + "/" + imageFileInfo.fileName();
    QFile::copy(imagePath, newImagePath);

    QString cfolderPath = objFolderPath + "/" + baseName + ".txt";
    QFile txtFile(cfolderPath);

    if (txtFile.open(QIODevice::Append | QIODevice::ReadWrite)) {
        QTextStream out(&txtFile);
        for (int j = 0; j < dinamicButtons.size(); ++j) {
            int size = std::min(dinamicButtons[j].firstc.size(), dinamicButtons[j].secondc.size());
            for (int k = 0; k < size; ++k) {
                QPointF first = dinamicButtons[j].firstc[k];
                QPointF second = dinamicButtons[j].secondc[k];

                QPointF center((first.x() + second.x()) / 2, (first.y() + second.y()) / 2);
                qreal width = std::abs(first.x() - second.x());
                qreal height = std::abs(first.y() - second.y());

                out << QString::number(j) << " " << center.x() << " " << center.y() << " " << width << " " << height << "\n";

            }
        }
        txtFile.close();
        ui.label->setText(QString("Created a file named: %1").arg(baseName + ".txt"));
        qDebug() << "Created a file named: " << baseName + ".txt";
    }
    else {
        ui.label->setText("Failed to create the file.");
        qDebug() << "Failed to create the file.";
    }
}

//---------------------------------------------DARKNET MANAGMENT----------------------
void interface::runPythonScript(const QString& scriptPath, QString script) {
    QProcess process;
    // Defina o diretório de trabalho para a pasta darknet
    process.setWorkingDirectory(scriptPath);

    // Comando para executar o script Python
    QString command = "python";
    QStringList arguments;
    arguments << script;

    // Conecte sinais para capturar saída e erros
    connect(&process, &QProcess::readyReadStandardOutput, [&]() {
        qDebug() << "Logs: " << process.readAllStandardOutput();
        });

    connect(&process, &QProcess::readyReadStandardError, [&]() {
        qDebug() << "Erro do processo: " << process.readAllStandardError();
        });

    // Inicie o processo
    process.start(command, arguments);

    // Aguarde o término do processo
    if (!process.waitForFinished()) {
        qDebug() << "Erro ao executar o processo: " << process.errorString();
    }
    else {
        qDebug() << "Processo terminado com sucesso.";
    }
}

void interface::runDarknetexe(const QString& scriptPath) {
    QString script = scriptPath;

    // Define the absolute path to darknet.exe
    QString absolutePathToDarknet = script + "/darknet.exe";
    qDebug() << "Initial absolutePathToDarknet:" << absolutePathToDarknet;

    // Verify if the file exists
    if (!QFile::exists(absolutePathToDarknet)) {
        qDebug() << "Error: The file" << absolutePathToDarknet << "does not exist.";
        return;
    }

    qDebug() << "File exists. Proceeding with process setup.";

    // Create and configure the QDialog for output
    QDialog* dialog = new QDialog(this);
    dialog->setWindowTitle("Logs");

    QTextEdit* outputEdit = new QTextEdit(dialog);
    outputEdit->setReadOnly(true);

    QPushButton* closeButton = new QPushButton("Close", dialog);

    QVBoxLayout* layout = new QVBoxLayout(dialog);
    layout->addWidget(outputEdit);
    layout->addWidget(closeButton);
    dialog->setLayout(layout);

    dialog->adjustSize();

    qDebug() << "Dialog setup complete. Creating QProcess.";

    // Create and configure the QProcess
    QProcess* process = new QProcess(this);

    // Verify and set the working directory
    QString workingDirectory = script;
    qDebug() << "Setting working directory to:" << workingDirectory;
    process->setWorkingDirectory(workingDirectory);

    // Check if the working directory is valid
    if (!QDir(workingDirectory).exists()) {
        qDebug() << "Error: The working directory" << workingDirectory << "does not exist.";
        outputEdit->append("<font color='red'>Error: The working directory does not exist.</font>");
        dialog->exec();
        delete process;  // Ensure proper cleanup
        return;
    }

    /*QStringList arguments;
    arguments << "detector" << "train"
        << "data/obj.data"
        << "cfg/yolov4-obj.cfg";*/

    QStringList arguments;
    arguments << "detector" << "train"
        << "data/obj.data"
        << "cfg/yolov4-obj.cfg"
        << "yolov4.conv.137";

    qDebug() << "Arguments for process:" << arguments;

    // Connect signals for capturing output and errors
    QObject::connect(process, &QProcess::readyReadStandardOutput, [process, outputEdit]() {
        QString output = process->readAllStandardOutput();
        qDebug() << "Standard Output:" << output;
        outputEdit->append(output);
        });

    QObject::connect(process, &QProcess::readyReadStandardError, [process, outputEdit]() {
        QString error = process->readAllStandardError();
        qDebug() << "Standard Error:" << error;
        outputEdit->append("<font color='red'>" + error + "</font>");
        });

    // Start the process
    qDebug() << "Starting process with path:" << absolutePathToDarknet;
    process->start(absolutePathToDarknet, arguments);

    if (!process->waitForStarted()) {
        qDebug() << "Error starting process:" << process->errorString();
        outputEdit->append("<font color='red'>Error starting process: " + process->errorString() + "</font>");
        dialog->exec();
        delete process;  // Ensure proper cleanup
        return;
    }

    qDebug() << "Process started successfully.";

    // Connect the close button to terminate the process
    QObject::connect(closeButton, &QPushButton::clicked, [process, dialog]() {
        qDebug() << "Close button clicked. Terminating process.";
        process->terminate();
        if (!process->waitForFinished(3000)) {
            process->kill();
            qDebug() << "Process killed.";
        }
        dialog->accept();
        });

    // Show the dialog
    dialog->exec();

    // After dialog is closed, check if the process is still running and terminate if needed
    if (process->state() != QProcess::NotRunning) {
        qDebug() << "Process still running after dialog closed. Terminating.";
        process->terminate();
        if (!process->waitForFinished(3000)) {
            process->kill();
            qDebug() << "Process killed after dialog closed.";
        }
    }

    delete process;
    qDebug() << "Process and dialog cleaned up.";
}

QString interface::duplicatedarknet(QString duplicate) {
    if (duplicate.contains("/darknet/darknet")) {
        duplicate = duplicate.replace("/darknet/darknet", "/darknet");
    }
    return duplicate;
}

void interface::on_train_clicked() {

    // Disable the Test button while training
    ui.test->setEnabled(false);

    qDebug() << "yolov4 cfg config";
    yolov4Config();

    qDebug() << "Python script exe:  " << darknetfolder;
    runPythonScript(darknetfolder, "process.py");

    qDebug() << " Darknet.exe";
    runDarknetexe(darknetfolder);

    // Re-enable the Test button after training
    ui.test->setEnabled(true);
}

void interface::processDarknetOutput() {
    QProcess* process = qobject_cast<QProcess*>(sender());
    if (process) {
        QString output = process->readAllStandardOutput();
        qDebug() << "Darknet process output:" << output;

        // Caminho para o arquivo de resultados
        QString resultFilePath = QDir::toNativeSeparators(darknetfolder + "/data/results.txt");

        // Abrir o arquivo em modo de escrita, o que irá truncar o arquivo automaticamente
        QFile resultFile(resultFilePath);
        if (resultFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&resultFile);
            out << output;  // Escreve a saída no arquivo, limpando o conteúdo anterior
            resultFile.close();
            qDebug() << "Output written to results file.";
        }
        else {
            qDebug() << "Failed to open results file for writing:" << resultFile.errorString();
        }

        // Verifica se o script Python está no caminho correto e é executável
        runPythonScript(darknetfolder + "/data", "resultsfilter.py");
        addCoordinatesToFile();
        loadCoords(scene);  // Use the main scene
    }
}

void interface::yolov4ComboBox() {
    // Save the current selection
    QString currentSelection = ui.comboBox->currentText();

    // Clear the existing items to avoid duplication
    ui.comboBox->clear();

    // Caminho absoluto para a pasta backup
    QString backupFolderPath = darknetfolder + "/backup";

    // Add "no target" if the directory path is empty
    if (darknetfolder.isEmpty()) {
        ui.comboBox->addItem("No weights");
        weights = ""; // Set weights to empty if the directory is not configured
    }
    else {
        // Open the folder and list all files
        QDir dir(backupFolderPath);
        if (!dir.exists()) {
            qDebug() << "Error: The directory" << backupFolderPath << "does not exist.";
            ui.comboBox->addItem("no target");
        }
        else {
            QFileInfoList fileInfoList = dir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot);
            if (fileInfoList.isEmpty()) {
                ui.comboBox->addItem("no target");
            }
            else {
                foreach(const QFileInfo & fileInfo, fileInfoList) {
                    ui.comboBox->addItem(fileInfo.fileName());
                }

                // Restore the previous selection if it exists
                int index = ui.comboBox->findText(currentSelection);
                if (index != -1) {
                    ui.comboBox->setCurrentIndex(index);
                }
                else {
                    // Default to the first item if the previous selection is not found
                    ui.comboBox->setCurrentIndex(0);
                }

                // Update weights based on the restored selection
                QString selectedFileName = ui.comboBox->currentText();
                if (selectedFileName != "no target") {
                    weights = QDir(backupFolderPath).filePath(selectedFileName);
                    qDebug() << "Weights initially set to:" << weights;
                }
                else {
                    weights = "";
                }
            }
        }
    }

    // Connect the signal for selection change to update weights
    connect(ui.comboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this, backupFolderPath]() {
        QString selectedFileName = ui.comboBox->currentText();
        if (selectedFileName != "no target") {
            weights = QDir(backupFolderPath).filePath(selectedFileName);
        }
        else {
            weights = "";
        }
        qDebug() << "Weights updated to:" << weights;
        });

    // Update the layout
    ui.buttongridLayout->update();
}

void interface::yolov4Config() {
    // Caminho absoluto para os arquivos
    QString dataFilePath = darknetfolder + "/data/obj.data";
    QString configFilePath = darknetfolder + "/cfg/yolov4-obj.cfg";

    qDebug() << "dataFilePath:" << dataFilePath << "configFilePath:" << configFilePath;

    // Lê o número de classes do arquivo obj.data
    int numClasses = 0;
    QFile dataFile(dataFilePath);
    if (dataFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&dataFile);
        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            if (line.startsWith("classes =")) {
                numClasses = line.split("=").last().trimmed().toInt();
                qDebug() << "Número de classes encontrado:" << numClasses;
                break;
            }
        }
        dataFile.close();
    }
    else {
        qDebug() << "Erro ao abrir o arquivo" << dataFilePath;
        return;
    }

    // Modifica o arquivo yolov4-obj.cfg
    QFile configFile(configFilePath);
    if (!configFile.open(QIODevice::ReadWrite | QIODevice::Text)) {
        qDebug() << "Erro ao abrir o arquivo" << configFilePath;
        return;
    }

    // Lê todo o conteúdo do arquivo
    QTextStream in(&configFile);
    QString content = in.readAll();
    configFile.seek(0);  // Volta ao início do arquivo para reescrever

    // Atualiza subdivisions
    content.replace(QRegularExpression("subdivisions=\\d+"), "subdivisions=64");
    qDebug() << "Atualizando subdivisions para: 64";

    // Atualiza max_batches
    int maxBatches = 2000 * numClasses;
    content.replace(QRegularExpression("max_batches=\\d+"), QString("max_batches=%1").arg(maxBatches));
    qDebug() << "Atualizando max_batches para:" << maxBatches;

    // Atualiza steps (já está correto)
    int eightyPercent = static_cast<int>(maxBatches * 0.8);
    int ninetyPercent = static_cast<int>(maxBatches * 0.9);
    content.replace(QRegularExpression("steps=\\d+,\\d+"), QString("steps=%1,%2").arg(eightyPercent).arg(ninetyPercent));
    qDebug() << "Atualizando steps para:" << eightyPercent << "," << ninetyPercent;

    // Atualiza apenas o filtro antes do bloco [yolo]
    QRegularExpression yoloBlockPattern("\\[yolo\\]");
    QRegularExpression convBlockPattern("\\[convolutional\\]");
    QRegularExpression filtersPattern("filters=\\d+");

    QRegularExpressionMatch yoloMatch = yoloBlockPattern.match(content);
    while (yoloMatch.hasMatch()) {
        int yoloPos = yoloMatch.capturedStart();

        // Encontra o bloco [convolutional] mais próximo antes do bloco [yolo]
        QRegularExpressionMatch convMatch = convBlockPattern.match(content, 0);
        int lastConvPos = -1;

        while (convMatch.hasMatch() && convMatch.capturedStart() < yoloPos) {
            lastConvPos = convMatch.capturedStart();
            convMatch = convBlockPattern.match(content, convMatch.capturedEnd());
        }

        if (lastConvPos != -1) {
            // Atualiza o filters deste bloco [convolutional]
            QRegularExpressionMatch filterMatch = filtersPattern.match(content, lastConvPos);
            if (filterMatch.hasMatch() && filterMatch.capturedStart() < yoloPos) {
                content.replace(filterMatch.capturedStart(), filterMatch.capturedLength(), QString("filters=%1").arg((numClasses + 5) * 3));
                qDebug() << "Atualizando filters para:" << (numClasses + 5) * 3 << "no bloco antes de" << yoloMatch.capturedStart();
            }
        }

        // Continua procurando o próximo bloco [yolo]
        yoloMatch = yoloBlockPattern.match(content, yoloPos + 1);
    }

    // Atualiza classes dentro do bloco [yolo]
    content.replace(QRegularExpression("classes=\\d+"), QString("classes=%1").arg(numClasses));
    qDebug() << "Atualizando classes para:" << numClasses;

    // Escreve o conteúdo modificado de volta ao arquivo
    configFile.resize(0); // Limpa o conteúdo do arquivo
    QTextStream out(&configFile);
    out << content;
    configFile.close();

    qDebug() << "Arquivo de configuração atualizado com sucesso.";
}

void interface::on_test_clicked() {
    // Alterna o valor de darknetDetect
    darknetDetect = !darknetDetect;
    yolov4ComboBox();

    // Atualiza o texto do rótulo para "Wait"
    ui.label->setText("Wait");

    if (darknetDetect) {
        // Inicia o processo de inicialização do Darknet
        initializeDarknet();

        // Usa um temporizador para aguardar antes de atualizar o texto
        QTimer::singleShot(7000, [this]() {
            // Atualiza o texto do rótulo com o valor atual de darknetDetect
            QString status = darknetDetect ? "Darknet detect enabled" : "Darknet detect disabled";
            ui.label->setText(status);
            });
    }
    else {
        // Se o darknetDetect estiver desativado, apenas atualiza o texto do rótulo
        QString status = darknetDetect ? "Darknet detect enabled" : "Darknet detect disabled";
        ui.label->setText(status);
    }
}

void interface::initializeDarknet() {
    qDebug() << "initializeDarknet called.";

    QString darknetCmd = darknetfolder + "/darknet.exe";  // Nome do executável darknet
    QString cfgFile = "cfg/yolov4-obj.cfg";  // Caminho relativo para o cfg
    QString dataFile = "data/obj.data";  // Caminho relativo para o data

    QStringList args;
    args << "detector" << "test" << dataFile << cfgFile << weights << "-dont_show" << "-ext_output"<<"-map";

    inferprocess = new QProcess(this);

    // Conectar sinais de erro
    connect(inferprocess, &QProcess::readyReadStandardOutput, this, &interface::processDarknetOutput);

    // Definir o diretório de trabalho
    QString workingDir = QDir::toNativeSeparators(darknetfolder);  // Convertendo para o formato do Windows
    inferprocess->setWorkingDirectory(workingDir);

    qDebug() << "Working directory set to:" << workingDir;

    // Verificar se o caminho completo para o executável e arquivos está correto
    qDebug() << "Starting Darknet process with command:" << darknetCmd << "and arguments:" << args;

    inferprocess->start(darknetCmd, args);

    if (!inferprocess->waitForStarted()) {
        qDebug() << "Failed to start Darknet process. Error:" << inferprocess->errorString();
    }
}

void interface::objdatafilechange() {
    QString objDataFilePath = darknetfolder + "/darknet/data/obj.data";
    QString objNamesFilePath = darknetfolder + "/darknet/data/obj.names";

    objDataFilePath = duplicatedarknet(objDataFilePath);
    objNamesFilePath = duplicatedarknet(objNamesFilePath);

    // Atualiza o arquivo obj.data.txt
    QFile dataFile(objDataFilePath);
    if (!dataFile.exists()) {
        qDebug() << "File does not exist:" << objDataFilePath;
        return;
    }

    if (!dataFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Failed to open file for reading:" << objDataFilePath;
        return;
    }

    QString content;
    QTextStream in(&dataFile);
    while (!in.atEnd()) {
        QString line = in.readLine();
        if (line.startsWith("classes =")) {
            line = "classes = " + QString::number(dinamicButtons.size());
        }
        content += line + "\n";
    }
    dataFile.close();

    if (!dataFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        qDebug() << "Failed to open file for writing:" << objDataFilePath;
        return;
    }

    QTextStream out(&dataFile);
    out << content;
    dataFile.close();

    // Atualiza o arquivo obj.names
    QFile namesFile(objNamesFilePath);
    if (!namesFile.exists()) {
        qDebug() << "File does not exist:" << objNamesFilePath;
        return;
    }

    if (!namesFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        qDebug() << "Failed to open file for writing:" << objNamesFilePath;
        return;
    }

    QTextStream namesOut(&namesFile);
    for (const auto& button : dinamicButtons) {
        namesOut << button.button->text() << "\n";
    }
    namesFile.close();
}



#endif // INTERFACE_CPP
