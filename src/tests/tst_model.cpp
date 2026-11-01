#include <QCoreApplication>
#include <QtTest>

#include <QAbstractItemModelTester>
#include <QTreeView>

#include "clipmodel.h"

class model : public QObject
{
    Q_OBJECT

public:
    model();
    ~model();

private slots:
    void test_case1();

private:
    ClipModel* m_clipModel;
};

model::model() {}

model::~model() {}

void model::test_case1()
{
    m_clipModel= new ClipModel();

    QTreeView* tree= new QTreeView();
    tree->setModel(m_clipModel);
    tree->setVisible(true);

    new QAbstractItemModelTester(m_clipModel);

    m_clipModel->insertClip(0, 1000);

    QCOMPARE(m_clipModel->rowCount(), 1);
    QCOMPARE(m_clipModel->rowCount(m_clipModel->index(0, 0, QModelIndex())), 0);

    m_clipModel->insertClip(2000, 3000);

    QCOMPARE(m_clipModel->rowCount(), 2);
    QCOMPARE(m_clipModel->rowCount(m_clipModel->index(1, 0, QModelIndex())), 0);

    delete m_clipModel;
}

QTEST_MAIN(model)

#include "tst_model.moc"
