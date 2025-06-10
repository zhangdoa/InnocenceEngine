#ifndef INNOVIEWPORT_H
#define INNOVIEWPORT_H

#include <QWidget>
#include <QResizeEvent>
#include <QApplication>
#include <QTimer>
#include "../Engine/Engine.h"

class ViewportEventFilter : public QObject
{
	Q_OBJECT

public:
	explicit ViewportEventFilter(WId hwnd, QObject* parent = nullptr)
		: m_HWND(hwnd)
		, QObject(parent)
	{
	}

protected:
	bool eventFilter(QObject* obj, QEvent* event) override;

private:
	WId m_HWND;
};

class Viewport : public QWidget
{
	Q_OBJECT

public:
	explicit Viewport(QWidget* parent = nullptr);
	virtual ~Viewport() override;

	void initialize();

	ViewportEventFilter* m_viewportEventFilter;

protected:
	virtual QPaintEngine* paintEngine() const override { return NULL; }

	void showEvent(QShowEvent* showEvent) override;

	void resizeEvent(QResizeEvent* resizeEvent) override;

private:
	void Resize(float width, float height);
	std::unique_ptr<Inno::Engine> m_pEngine;
	WId m_HWND;
};

#endif // INNOVIEWPORT_H
