#ifndef DIRECTORYLISTVIEWER_H
#define DIRECTORYLISTVIEWER_H

#include <QListView>
#include <QFileSystemModel>
#include <QMenu>
#include <QShortcut>

class DirectoryListViewer : public QListView
{
	Q_OBJECT
public:
    explicit DirectoryListViewer(QWidget *parent = nullptr);

    void Initialize();

    QString GetRootPath();
	void SetRootPath(QString path);
	QString GetFilePath(QModelIndex index);

private:
	QString GetSelectionPath();

	QFileSystemModel* m_fileModel;
	QString m_rootDir;

signals:

public slots:
	void InteractiveWithFile(QModelIndex index);
    void OpenFileMenu(QModelIndex index);
	void SaveScene();
};

#endif // DIRECTORYLISTVIEWER_H
