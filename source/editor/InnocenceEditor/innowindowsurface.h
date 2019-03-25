#ifndef INNOWINDOWSURFACE_H
#define INNOWINDOWSURFACE_H

#include <QWidget>
#include <QResizeEvent>
#include <QApplication>
#include <QTimer>

#include "../../engine/system/CoreSystem.h"
#include "../../game/GameInstance.h"

class InnoWindowSurface : public QWidget
{
    Q_OBJECT

public:
    explicit InnoWindowSurface(QWidget* parent);
    virtual ~InnoWindowSurface() override;

    void initialize();

protected:
    virtual QPaintEngine* paintEngine() const override { return NULL; }

    void paintEvent(QPaintEvent* paintEvent) override;

    void showEvent(QShowEvent* showEvent) override;

    void resizeEvent(QResizeEvent* resizeEvent) override;

    bool nativeEvent(const QByteArray &eventType, void *message, long *result) override;

private:
    QTimer* m_timerUpdate;

    InnoCoreSystem* m_CoreSystem;
    GameInstance* m_GameInstance;

    void Resize(float width, float height);

public slots:
    void Update();
};

#endif // INNOWINDOWSURFACE_H
