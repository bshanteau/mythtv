#ifndef MEDIASERVERUTIL_H_
#define MEDIASERVERUTIL_H_

using namespace std;

// c++
#include <utility>

// qt
#include <QDateTime>
#include <QString>

#include "mythprotoserverexp.h"
#include "programinfo.h"
#include "referencecounter.h"

class DeleteThread;

class PROTOSERVER_PUBLIC DeleteHandler : public ReferenceCounter
{
  public:
    DeleteHandler(void);
    explicit DeleteHandler(const QString& filename);
   ~DeleteHandler(void) override;

    void Close(void);

    QString     GetPath(void)           { return m_path;    }
    int         GetFD(void)             { return m_fd;      }
    off_t       GetSize(void)           { return m_size;    }
    QDateTime   GetWait(void)           { return m_wait;    }

    void        SetPath(QString path)   { m_path= std::move(path);     }

    virtual void DeleteSucceeded(void)  {};
    virtual void DeleteFailed(void)     {};

    friend class DeleteThread;

  private:
    QString     m_path;
    int         m_fd   { -1 };
    off_t       m_size {  0 };
    QDateTime   m_wait;

};

QString GetPlaybackURL(ProgramInfo *pginfo, bool storePath = true);

#endif
