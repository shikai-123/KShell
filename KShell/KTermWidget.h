#pragma once

#include <QWidget>
#include <QFont>
#include <QColor>
#include <QByteArray>
#include <QVector>
#ifdef small
#undef small
#endif
#include <vterm.h>

class KTermWidget : public QWidget
{
    Q_OBJECT
public:
    explicit KTermWidget(QWidget *parent = nullptr);
    ~KTermWidget();

    void receiveData(const QByteArray &data);
    void reset();
    void setDefaultFont(const QFont &font);
    int getRows() const { return m_rows; }
    int getCols() const { return m_cols; }
    void setDebugEnabled(bool enabled) { m_debugEnabled = enabled; }

    static int cbDamage(VTermRect rect, void *user);
    static int cbMoveRect(VTermRect dest, VTermRect src, void *user);
    static int cbMoveCursor(VTermPos pos, VTermPos oldpos, int visible, void *user);
    static int cbSetTermProp(VTermProp prop, VTermValue *val, void *user);
    static int cbBell(void *user);
    static int cbResize(int rows, int cols, void *user);
    static int cbSbPushline(int cols, const VTermScreenCell *cells, void *user);
    static int cbSbPopline(int cols, VTermScreenCell *cells, void *user);

signals:
    void sendData(const QByteArray &data);
    void sizeChanged(int rows, int cols);

protected:
    void paintEvent(QPaintEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

private:
    VTerm *m_vterm;
    VTermScreen *m_screen;
    VTermState *m_state;

    QFont m_font;
    int m_cellWidth;
    int m_cellHeight;
    int m_ascent;
    int m_rows;
    int m_cols;

    VTermPos m_cursorPos;
    bool m_cursorVisible;
    bool m_cursorBlink;

    QColor m_defaultFg;
    QColor m_defaultBg;
    bool m_debugEnabled = false;

    QVector<QVector<VTermScreenCell>> m_scrollBuffer;
    int m_scrollOffset = 0;
    int m_maxScrollLines = 5000;

    VTermPos m_selStart;
    VTermPos m_selEnd;
    bool m_selecting = false;
    bool m_hasSelection = false;

    int m_mouseMode = VTERM_PROP_MOUSE_NONE;

    void initVTerm(int rows, int cols);
    void updateCellSize();
    void updateTermSize();
    void flushOutput();
    QColor vtermColorToQColor(const VTermColor &color) const;
    VTermModifier qtModifiersToVTerm(Qt::KeyboardModifiers mods) const;
    VTermKey qtKeyToVTermKey(int qtKey) const;

    VTermPos mouseToCellPos(const QPoint &pos) const;
    QString getSelectedText() const;
    void copySelection();
    void pasteClipboard();
    void selectAll();
    void resetScrollOffset();
};
