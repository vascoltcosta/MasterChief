#ifndef REDODIALOG_H
#define REDODIALOG_H

#include "interface.h"
#include <QGraphicsRectItem>
#include <QVBoxLayout>
#include <QImage>
#include <QPushButton>
#include <QDebug>
#include <QDialog>
#include <QGraphicsPixmapItem>
#include <QGraphicsView>

class RedoDialog : public QDialog {
    Q_OBJECT

public:
    RedoDialog(QWidget* parent = nullptr)
        : QDialog(parent), currentSelectionIndex(-1) {
        QVBoxLayout* layout = new QVBoxLayout(this);

        graphicsViewRedo = new QGraphicsView(this);
        graphicsSceneRedo = new QGraphicsScene(this);
        graphicsViewRedo->setScene(graphicsSceneRedo);

        prevButton = new QPushButton("Previous Selection", this);
        redoButton = new QPushButton("Redo", this);
        nextButton = new QPushButton("Next Selection", this);

        layout->addWidget(graphicsViewRedo);
        layout->addWidget(prevButton);
        layout->addWidget(redoButton);
        layout->addWidget(nextButton);

        connect(prevButton, &QPushButton::clicked, this, &RedoDialog::showPreviousSelection);
        connect(nextButton, &QPushButton::clicked, this, &RedoDialog::showNextSelection);
        connect(redoButton, &QPushButton::clicked, this, &RedoDialog::redoSelection);
    }

    void setImage(const QImage& image) {
        this->image = image;
        QPixmap pixmap = QPixmap::fromImage(image);
        graphicsSceneRedo->clear();
        graphicsSceneRedo->addPixmap(pixmap);
        updateSelections();
    }

    void setSelections(const QList<QRectF>& selections) {
        this->selections = selections;
        updateSelections();
    }

private slots:
    void showPreviousSelection() {
        if (selections.isEmpty()) return;
        currentSelectionIndex = (currentSelectionIndex - 1 + selections.size()) % selections.size();
        updateSelections();
    }

    void showNextSelection() {
        if (selections.isEmpty()) return;
        currentSelectionIndex = (currentSelectionIndex + 1) % selections.size();
        updateSelections();
    }

    void redoSelection() {
        if (currentSelectionIndex == -1) return;

        // Trigger redo action (e.g., open a dialog for new selection)
        qDebug() << "Redo selection triggered for index:" << currentSelectionIndex;
        // This could be a method call to another dialog or function for actual redo
    }

private:
    void updateSelections() {
        graphicsSceneRedo->clear();
        if (!image.isNull()) {
            QPixmap pixmap = QPixmap::fromImage(image);
            graphicsSceneRedo->addPixmap(pixmap);
        }
        for (int i = 0; i < selections.size(); ++i) {
            QGraphicsRectItem* rectItem = new QGraphicsRectItem(selections[i]);
            rectItem->setPen(QPen(i == currentSelectionIndex ? Qt::green : Qt::red, 2));
            graphicsSceneRedo->addItem(rectItem);
        }
    }

    QGraphicsView* graphicsViewRedo;
    QGraphicsScene* graphicsSceneRedo;
    QPushButton* prevButton;
    QPushButton* redoButton;
    QPushButton* nextButton;
    QImage image;
    QList<QRectF> selections;
    int currentSelectionIndex;
};

#endif // REDODIALOG_H
