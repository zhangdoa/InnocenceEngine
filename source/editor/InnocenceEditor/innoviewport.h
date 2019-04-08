#ifndef INNOVIEWPORT_H
#define INNOVIEWPORT_H

#include <QWidget>
#include <QResizeEvent>
#include <QApplication>
#include <QTimer>
#include "../../engine/system/ICoreSystem.h"
#include "../../game/IGameInstance.h"

class ViewportEventFilter : public QObject
{
    Q_OBJECT

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
};

class InnoViewport : public QWidget
{
    Q_OBJECT

public:
    explicit InnoViewport(QWidget *parent = nullptr);
    virtual ~InnoViewport() override;

    void initialize();

    ViewportEventFilter* m_viewportEventFilter;

protected:
    virtual QPaintEngine* paintEngine() const override { return NULL; }

    void paintEvent(QPaintEvent* paintEvent) override;

    void showEvent(QShowEvent* showEvent) override;

    void resizeEvent(QResizeEvent* resizeEvent) override;

private:
    QTimer* m_timerUpdate;

    ICoreSystem* m_CoreSystem;
    IGameInstance* m_GameInstance;

    void Resize(float width, float height);

public slots:
    void Update();
};

#endif // INNOVIEWPORT_H
