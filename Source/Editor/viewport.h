#ifndef INNOVIEWPORT_H
#define INNOVIEWPORT_H

#include <QWidget>
#include <QResizeEvent>
#include <QApplication>
#include <QTimer>

class ViewportEventFilter : public QObject
{
	Q_OBJECT

protected:
	bool eventFilter(QObject *obj, QEvent *event) override;
};

class Viewport : public QWidget
{
	Q_OBJECT

public:
	explicit Viewport(QWidget *parent = nullptr);
	virtual ~Viewport() override;

	void initialize();

	ViewportEventFilter* m_viewportEventFilter;

protected:
	virtual QPaintEngine* paintEngine() const override { return NULL; }

	void showEvent(QShowEvent* showEvent) override;

	void resizeEvent(QResizeEvent* resizeEvent) override;

private:
	void Resize(float width, float height);
};

#endif // INNOVIEWPORT_H
