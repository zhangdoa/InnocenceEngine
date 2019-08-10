#ifndef INNOFILEEXPLORER_H
#define INNOFILEEXPLORER_H

#include <QListView>
#include <QFileSystemModel>

class InnoFileExplorer : public QListView
{
	Q_OBJECT
public:
	explicit InnoFileExplorer(QWidget *parent = nullptr);

	void initialize();

	void SetRootPath(QString path);
	QString GetFilePath(QModelIndex index);

private:
	void SetRootDirectory(const std::string& directory);
	QString GetRootPath();
	QString GetSelectionPath();

	QFileSystemModel* m_fileModel;
	QString m_rootDir;

signals:

public slots:
	void DoubleClick(QModelIndex index);
	void SaveScene();
};

#endif // INNOFILEEXPLORER_H
