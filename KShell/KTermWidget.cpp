#include "KTermWidget.h"
#include <QPainter>
#include <QKeyEvent>
#include <QResizeEvent>
#include <QWheelEvent>
#include <QFontMetrics>
#include <QApplication>
#include <QClipboard>
#include <QDebug>
#include <QMenu>
#include <QAction>
#include <QContextMenuEvent>
#include <cstring>

static const VTermScreenCallbacks s_screenCallbacks = {
    KTermWidget::cbDamage,
    KTermWidget::cbMoveRect,
    KTermWidget::cbMoveCursor,
    KTermWidget::cbSetTermProp,
    KTermWidget::cbBell,
    KTermWidget::cbResize,
    KTermWidget::cbSbPushline,
    KTermWidget::cbSbPopline,
    nullptr,
};

static const QColor s_base16Colors[16] = {
    QColor(0x00, 0x00, 0x00),
    QColor(0xcd, 0x00, 0x00),
    QColor(0x00, 0xcd, 0x00),
    QColor(0xcd, 0xcd, 0x00),
    QColor(0x00, 0x00, 0xee),
    QColor(0xcd, 0x00, 0xcd),
    QColor(0x00, 0xcd, 0xcd),
    QColor(0xe5, 0xe5, 0xe5),
    QColor(0x7f, 0x7f, 0x7f),
    QColor(0xff, 0x00, 0x00),
    QColor(0x00, 0xff, 0x00),
    QColor(0xff, 0xff, 0x00),
    QColor(0x5c, 0x5c, 0xff),
    QColor(0xff, 0x00, 0xff),
    QColor(0x00, 0xff, 0xff),
    QColor(0xff, 0xff, 0xff),
};

KTermWidget::KTermWidget(QWidget *parent)
    : QWidget(parent), m_vterm(nullptr), m_screen(nullptr), m_state(nullptr), m_cellWidth(0), m_cellHeight(0), m_ascent(0), m_rows(24), m_cols(80), m_cursorVisible(true), m_cursorBlink(true), m_defaultFg(0xe5, 0xe5, 0xe5), m_defaultBg(0x00, 0x00, 0x00)
{
    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_InputMethodEnabled, true);
    setAttribute(Qt::WA_KeyCompression, false);
    setMinimumSize(200, 100);

    m_font.setFamily(QStringLiteral("Microsoft YaHei Mono"));
    m_font.setPointSize(10);
    m_font.setStyleHint(QFont::Monospace);
    m_font.setFixedPitch(true);
    m_font.setKerning(false);

    updateCellSize();

    if (m_cellWidth > 0 && m_cellHeight > 0)
    {
        m_rows = qMax(1, height() / m_cellHeight);
        m_cols = qMax(1, width() / m_cellWidth);
    }

    initVTerm(m_rows, m_cols);
}

KTermWidget::~KTermWidget()
{
    if (m_vterm)
        vterm_free(m_vterm);
}

void KTermWidget::setDefaultFont(const QFont &font)
{
    m_font = font;
    m_font.setFixedPitch(true);
    m_font.setKerning(false);
    updateCellSize();
    updateTermSize();
}

void KTermWidget::updateCellSize()
{
    QFontMetrics fm(m_font);
    m_cellWidth = fm.horizontalAdvance(QLatin1Char('M'));
    m_cellHeight = fm.height();
    m_ascent = fm.ascent();
    if (m_cellWidth < 1)
        m_cellWidth = 8;
    if (m_cellHeight < 1)
        m_cellHeight = 16;
}

void KTermWidget::initVTerm(int rows, int cols)
{
    if (m_vterm)
        vterm_free(m_vterm);

    m_vterm = vterm_new(rows, cols);
    vterm_set_utf8(m_vterm, 1);

    m_screen = vterm_obtain_screen(m_vterm);
    vterm_screen_set_callbacks(m_screen, &s_screenCallbacks, this);
    vterm_screen_set_damage_merge(m_screen, VTERM_DAMAGE_ROW);
    vterm_screen_enable_altscreen(m_screen, 1);
    vterm_screen_reset(m_screen, 1);

    m_state = vterm_obtain_state(m_vterm);
    VTermColor fg, bg;
    vterm_color_rgb(&fg, 0xe5, 0xe5, 0xe5);
    vterm_color_rgb(&bg, 0x00, 0x00, 0x00);
    vterm_state_set_default_colors(m_state, &fg, &bg);
    vterm_screen_set_default_colors(m_screen, &fg, &bg);

    m_rows = rows;
    m_cols = cols;
    memset(&m_cursorPos, 0, sizeof(m_cursorPos));
}

void KTermWidget::updateTermSize()
{
    if (m_cellWidth < 1 || m_cellHeight < 1)
        return;

    int newRows = qMax(1, height() / m_cellHeight);
    int newCols = qMax(1, width() / m_cellWidth);

    if (newRows != m_rows || newCols != m_cols)
    {
        m_scrollBuffer.clear();
        m_scrollOffset = 0;
        m_rows = newRows;
        m_cols = newCols;
        if (m_vterm)
        {
            vterm_set_size(m_vterm, m_rows, m_cols);
            vterm_screen_flush_damage(m_screen);
        }
    }
}

void KTermWidget::receiveData(const QByteArray &data)
{
    if (!m_vterm || data.isEmpty())
        return;

    if (m_debugEnabled)
        qDebug() << "[KTerm] receiveData:" << data.size() << "bytes:" << data.toHex().left(200);

    vterm_input_write(m_vterm, data.constData(), data.size());
    vterm_screen_flush_damage(m_screen);
    update();
}

void KTermWidget::reset()
{
    m_scrollBuffer.clear();
    m_scrollOffset = 0;
    m_hasSelection = false;
    m_selecting = false;
    initVTerm(m_rows, m_cols);
    update();
}

QColor KTermWidget::vtermColorToQColor(const VTermColor &color) const
{
    if (VTERM_COLOR_IS_DEFAULT_FG(&color))
        return m_defaultFg;
    if (VTERM_COLOR_IS_DEFAULT_BG(&color))
        return m_defaultBg;

    if (VTERM_COLOR_IS_RGB(&color))
        return QColor(color.rgb.red, color.rgb.green, color.rgb.blue);

    if (VTERM_COLOR_IS_INDEXED(&color))
    {
        uint8_t idx = color.indexed.idx;
        if (idx < 16)
            return s_base16Colors[idx];
        if (idx < 232)
        {
            idx -= 16;
            int r = (idx / 36) * 51;
            int g = ((idx % 36) / 6) * 51;
            int b = (idx % 6) * 51;
            return QColor(r, g, b);
        }
        if (idx < 256)
        {
            int gray = 8 + (idx - 232) * 10;
            return QColor(gray, gray, gray);
        }
    }

    return m_defaultFg;
}

void KTermWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setFont(m_font);
    painter.setRenderHint(QPainter::TextAntialiasing, false);

    painter.fillRect(rect(), m_defaultBg);

    VTermScreenCell cell;
    QRect cellRect;
    int viewStart = m_scrollBuffer.size() - m_scrollOffset;

    for (int visualRow = 0; visualRow < m_rows; visualRow++)
    {
        int contentRow = viewStart + visualRow;
        for (int col = 0; col < m_cols; col++)
        {
            if (contentRow < m_scrollBuffer.size())
            {
                const QVector<VTermScreenCell> &row = m_scrollBuffer.at(contentRow);
                if (col < row.size())
                    cell = row.at(col);
                else
                    memset(&cell, 0, sizeof(cell));
            }
            else
            {
                VTermPos pos;
                pos.row = contentRow - m_scrollBuffer.size();
                pos.col = col;
                if (!vterm_screen_get_cell(m_screen, pos, &cell))
                {
                    memset(&cell, 0, sizeof(cell));
                }
            }

            QColor fg = vtermColorToQColor(cell.fg);
            QColor bg = vtermColorToQColor(cell.bg);

            bool inSelection = false;
            if (m_hasSelection)
            {
                int absRow = contentRow;
                int selTop = m_selStart.row, selBot = m_selEnd.row;
                int selLeft = m_selStart.col, selRight = m_selEnd.col;
                if (selTop > selBot || (selTop == selBot && selLeft > selRight))
                {
                    qSwap(selTop, selBot);
                    qSwap(selLeft, selRight);
                }
                if (absRow >= selTop && absRow <= selBot)
                {
                    if (selTop == selBot)
                    {
                        inSelection = (col >= selLeft && col <= selRight);
                    }
                    else if (absRow == selTop)
                    {
                        inSelection = (col >= selLeft);
                    }
                    else if (absRow == selBot)
                    {
                        inSelection = (col <= selRight);
                    }
                    else
                    {
                        inSelection = true;
                    }
                }
            }

            if (cell.attrs.reverse ^ inSelection)
            {
                qSwap(fg, bg);
            }

            if (cell.attrs.bold && fg.lightness() < 200)
            {
                fg = fg.lighter(130);
            }

            int x = col * m_cellWidth;
            int y = visualRow * m_cellHeight;
            int charWidth = cell.width > 1 ? m_cellWidth * cell.width : m_cellWidth;

            cellRect.setRect(x, y, charWidth, m_cellHeight);

            painter.fillRect(cellRect, bg);

            uint32_t c = cell.chars[0];
            // 双宽度字符的续行格（libvterm 用 0xFFFFFFFF 标记）只画背景不画文字
            if (c != static_cast<uint32_t>(-1) && c >= 0x20 && c != 0x7f)
            {
                QString str;
                if (c < 0x10000)
                {
                    str = QChar(static_cast<ushort>(c));
                }
                else
                {
                    str = QString::fromUcs4(&c, 1);
                }
                painter.setPen(fg);
                painter.drawText(x, y + m_ascent, str);
            }

            if (cell.attrs.underline == VTERM_UNDERLINE_SINGLE)
            {
                painter.setPen(fg);
                painter.drawLine(x, y + m_cellHeight - 1, x + charWidth - 1, y + m_cellHeight - 1);
            }
        }
    }

    if (m_cursorVisible && m_scrollOffset == 0)
    {
        int cx = m_cursorPos.col * m_cellWidth;
        int cy = m_cursorPos.row * m_cellHeight;
        QColor cursorColor(0xc0, 0xc0, 0xc0);
        painter.setPen(QPen(cursorColor, 1));
        painter.drawRect(cx, cy, m_cellWidth - 1, m_cellHeight - 1);
    }
}

void KTermWidget::keyPressEvent(QKeyEvent *event)
{
    if (!m_vterm)
        return;

    resetScrollOffset();

    if (m_debugEnabled)
        qDebug() << "[KTerm] keyPress: key=" << event->key() << "text=" << event->text().toUtf8().toHex().left(40);

    if ((event->modifiers() & Qt::ControlModifier) && (event->modifiers() & Qt::ShiftModifier))
    {
        if (event->key() == Qt::Key_C)
        {
            copySelection();
            return;
        }
        if (event->key() == Qt::Key_V)
        {
            pasteClipboard();
            return;
        }
    }

    VTermModifier mod = qtModifiersToVTerm(event->modifiers());
    int qtKey = event->key();

    if (event->modifiers() & Qt::ControlModifier)
    {
        char ch = 0;
        if (qtKey >= Qt::Key_A && qtKey <= Qt::Key_Z)
            ch = static_cast<char>(qtKey - Qt::Key_A + 1);
        else if (qtKey == Qt::Key_Space)
            ch = 0;
        else if (qtKey == Qt::Key_BracketLeft)
            ch = 0x1b;
        else if (qtKey == Qt::Key_Backslash)
            ch = 0x1c;
        else if (qtKey == Qt::Key_BracketRight)
            ch = 0x1d;

        if (ch)
        {
            QByteArray data(&ch, 1);
            emit sendData(data);
            return;
        }
    }

    if (event->modifiers() & Qt::AltModifier)
    {
        QByteArray esc("\x1b", 1);
        QString text = event->text();
        if (!text.isEmpty())
        {
            emit sendData(esc + text.toUtf8());
            return;
        }
    }

    if (qtKey == Qt::Key_Return || qtKey == Qt::Key_Enter)
    {
        emit sendData(QByteArray("\r", 1));
        return;
    }

    VTermKey vkey = qtKeyToVTermKey(qtKey);
    if (vkey != VTERM_KEY_NONE)
    {
        vterm_keyboard_key(m_vterm, vkey, mod);

        char buf[32];
        size_t len;
        QByteArray output;
        while ((len = vterm_output_read(m_vterm, buf, sizeof(buf))) > 0)
        {
            output.append(buf, static_cast<int>(len));
        }
        if (!output.isEmpty())
            emit sendData(output);
        return;
    }

    QString text = event->text();
    if (!text.isEmpty())
    {
        QVector<uint> ucs4 = text.toUcs4();
        for (uint c : ucs4)
        {
            if (c >= 0x20)
            {
                vterm_keyboard_unichar(m_vterm, c, mod);
            }
        }

        char buf[32];
        size_t len;
        QByteArray output;
        while ((len = vterm_output_read(m_vterm, buf, sizeof(buf))) > 0)
        {
            output.append(buf, static_cast<int>(len));
        }
        if (!output.isEmpty())
            emit sendData(output);
    }
}

void KTermWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    updateCellSize();
    updateTermSize();
    update();
}

void KTermWidget::mousePressEvent(QMouseEvent *event)
{
    setFocus();
    if (event->button() == Qt::LeftButton)
    {
        m_selStart = mouseToCellPos(event->pos());
        m_selEnd = m_selStart;
        m_selecting = true;
        m_hasSelection = false;
        update();
    }
}

void KTermWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (m_selecting)
    {
        m_selEnd = mouseToCellPos(event->pos());
        if (m_selStart.row != m_selEnd.row || m_selStart.col != m_selEnd.col)
            m_hasSelection = true;
        update();
    }
}

void KTermWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && m_selecting)
    {
        m_selEnd = mouseToCellPos(event->pos());
        m_selecting = false;
        if (m_selStart.row != m_selEnd.row || m_selStart.col != m_selEnd.col)
            m_hasSelection = true;
        else
            m_hasSelection = false;
        update();
    }
}

void KTermWidget::wheelEvent(QWheelEvent *event)
{
    if (!m_vterm)
        return;

    int delta = event->angleDelta().y();
    int steps = delta / 120;
    if (steps == 0)
        steps = (delta > 0) ? 1 : -1;

    if (steps > 0)
    {
        m_scrollOffset = qMin(m_scrollOffset + steps * 3, m_scrollBuffer.size());
    }
    else if (steps < 0)
    {
        m_scrollOffset = qMax(m_scrollOffset + steps * 3, 0);
    }
    update();
}

QSize KTermWidget::sizeHint() const
{
    return QSize(m_cols * m_cellWidth, m_rows * m_cellHeight);
}

QSize KTermWidget::minimumSizeHint() const
{
    return QSize(20 * m_cellWidth, 5 * m_cellHeight);
}

VTermModifier KTermWidget::qtModifiersToVTerm(Qt::KeyboardModifiers mods) const
{
    int result = VTERM_MOD_NONE;
    if (mods & Qt::ShiftModifier)
        result |= VTERM_MOD_SHIFT;
    if (mods & Qt::AltModifier)
        result |= VTERM_MOD_ALT;
    if (mods & Qt::ControlModifier)
        result |= VTERM_MOD_CTRL;
    return static_cast<VTermModifier>(result);
}

VTermKey KTermWidget::qtKeyToVTermKey(int qtKey) const
{
    switch (qtKey)
    {
    case Qt::Key_Up:
        return VTERM_KEY_UP;
    case Qt::Key_Down:
        return VTERM_KEY_DOWN;
    case Qt::Key_Left:
        return VTERM_KEY_LEFT;
    case Qt::Key_Right:
        return VTERM_KEY_RIGHT;
    case Qt::Key_Insert:
        return VTERM_KEY_INS;
    case Qt::Key_Delete:
        return VTERM_KEY_DEL;
    case Qt::Key_Home:
        return VTERM_KEY_HOME;
    case Qt::Key_End:
        return VTERM_KEY_END;
    case Qt::Key_PageUp:
        return VTERM_KEY_PAGEUP;
    case Qt::Key_PageDown:
        return VTERM_KEY_PAGEDOWN;
    case Qt::Key_Return:
    case Qt::Key_Enter:
        return VTERM_KEY_ENTER;
    case Qt::Key_Tab:
        return VTERM_KEY_TAB;
    case Qt::Key_Backspace:
        return VTERM_KEY_BACKSPACE;
    case Qt::Key_Escape:
        return VTERM_KEY_ESCAPE;
    case Qt::Key_F1:
        return static_cast<VTermKey>(VTERM_KEY_FUNCTION(1));
    case Qt::Key_F2:
        return static_cast<VTermKey>(VTERM_KEY_FUNCTION(2));
    case Qt::Key_F3:
        return static_cast<VTermKey>(VTERM_KEY_FUNCTION(3));
    case Qt::Key_F4:
        return static_cast<VTermKey>(VTERM_KEY_FUNCTION(4));
    case Qt::Key_F5:
        return static_cast<VTermKey>(VTERM_KEY_FUNCTION(5));
    case Qt::Key_F6:
        return static_cast<VTermKey>(VTERM_KEY_FUNCTION(6));
    case Qt::Key_F7:
        return static_cast<VTermKey>(VTERM_KEY_FUNCTION(7));
    case Qt::Key_F8:
        return static_cast<VTermKey>(VTERM_KEY_FUNCTION(8));
    case Qt::Key_F9:
        return static_cast<VTermKey>(VTERM_KEY_FUNCTION(9));
    case Qt::Key_F10:
        return static_cast<VTermKey>(VTERM_KEY_FUNCTION(10));
    case Qt::Key_F11:
        return static_cast<VTermKey>(VTERM_KEY_FUNCTION(11));
    case Qt::Key_F12:
        return static_cast<VTermKey>(VTERM_KEY_FUNCTION(12));
    default:
        return VTERM_KEY_NONE;
    }
}

int KTermWidget::cbDamage(VTermRect rect, void *user)
{
    KTermWidget *self = static_cast<KTermWidget *>(user);
    if (self)
    {
        if (self->m_debugEnabled)
            qDebug() << "[KTerm] damage: rect(" << rect.start_row << "," << rect.start_col << ")-(" << rect.end_row << "," << rect.end_col << ")";
        self->update();
    }
    return 1;
}

int KTermWidget::cbMoveRect(VTermRect dest, VTermRect src, void *user)
{
    Q_UNUSED(dest);
    Q_UNUSED(src);
    Q_UNUSED(user);
    return 1;
}

int KTermWidget::cbMoveCursor(VTermPos pos, VTermPos oldpos, int visible, void *user)
{
    Q_UNUSED(oldpos);
    KTermWidget *self = static_cast<KTermWidget *>(user);
    if (self)
    {
        self->m_cursorPos = pos;
        self->m_cursorVisible = (visible != 0);
        if (self->m_debugEnabled)
            qDebug() << "[KTerm] cursor moved to (" << pos.row << "," << pos.col << ")";
    }
    return 1;
}

int KTermWidget::cbSetTermProp(VTermProp prop, VTermValue *val, void *user)
{
    KTermWidget *self = static_cast<KTermWidget *>(user);
    if (!self)
        return 1;

    switch (prop)
    {
    case VTERM_PROP_CURSORVISIBLE:
        self->m_cursorVisible = val->boolean;
        self->update();
        break;
    case VTERM_PROP_CURSORBLINK:
        self->m_cursorBlink = val->boolean;
        break;
    case VTERM_PROP_ALTSCREEN:
        self->update();
        break;
    default:
        break;
    }
    return 1;
}

int KTermWidget::cbBell(void *user)
{
    Q_UNUSED(user);
    QApplication::beep();
    return 1;
}

int KTermWidget::cbResize(int rows, int cols, void *user)
{
    KTermWidget *self = static_cast<KTermWidget *>(user);
    if (self)
    {
        self->m_rows = rows;
        self->m_cols = cols;
    }
    return 1;
}

int KTermWidget::cbSbPushline(int cols, const VTermScreenCell *cells, void *user)
{
    KTermWidget *self = static_cast<KTermWidget *>(user);
    if (!self)
        return 1;

    QVector<VTermScreenCell> row(cols);
    for (int i = 0; i < cols; i++)
        row[i] = cells[i];

    self->m_scrollBuffer.append(row);

    while (self->m_scrollBuffer.size() > self->m_maxScrollLines)
        self->m_scrollBuffer.removeFirst();

    self->m_scrollOffset = qMin(self->m_scrollOffset, self->m_scrollBuffer.size());
    return 1;
}

int KTermWidget::cbSbPopline(int cols, VTermScreenCell *cells, void *user)
{
    KTermWidget *self = static_cast<KTermWidget *>(user);
    if (!self || self->m_scrollBuffer.isEmpty())
        return 0;

    QVector<VTermScreenCell> &row = self->m_scrollBuffer.last();
    int copyCols = qMin(cols, row.size());
    for (int i = 0; i < copyCols; i++)
        cells[i] = row[i];
    for (int i = copyCols; i < cols; i++)
        memset(&cells[i], 0, sizeof(VTermScreenCell));

    self->m_scrollBuffer.removeLast();
    self->m_scrollOffset = qMin(self->m_scrollOffset, self->m_scrollBuffer.size());
    return 1;
}

VTermPos KTermWidget::mouseToCellPos(const QPoint &pos) const
{
    VTermPos p;
    p.col = qBound(0, pos.x() / m_cellWidth, m_cols - 1);
    int visualRow = qBound(0, pos.y() / m_cellHeight, m_rows - 1);
    int viewStart = m_scrollBuffer.size() - m_scrollOffset;
    p.row = viewStart + visualRow;
    return p;
}

QString KTermWidget::getSelectedText() const
{
    if (!m_hasSelection || !m_vterm)
        return QString();

    int selTop = m_selStart.row;
    int selBot = m_selEnd.row;
    int selLeft = m_selStart.col;
    int selRight = m_selEnd.col;

    if (selTop > selBot || (selTop == selBot && selLeft > selRight))
    {
        qSwap(selTop, selBot);
        qSwap(selLeft, selRight);
    }

    QString result;

    for (int absRow = selTop; absRow <= selBot; absRow++)
    {
        int colStart = (absRow == selTop) ? selLeft : 0;
        int colEnd = (absRow == selBot) ? selRight : (m_cols - 1);

        for (int col = colStart; col <= colEnd; col++)
        {
            VTermScreenCell cell;
            memset(&cell, 0, sizeof(cell));

            if (absRow < m_scrollBuffer.size())
            {
                const QVector<VTermScreenCell> &row = m_scrollBuffer.at(absRow);
                if (col < row.size())
                    cell = row.at(col);
            }
            else
            {
                VTermPos pos;
                pos.row = absRow - m_scrollBuffer.size();
                pos.col = col;
                vterm_screen_get_cell(m_screen, pos, &cell);
            }

            uint32_t c = cell.chars[0];
            // 跳过双宽度字符的续行格
            if (c != static_cast<uint32_t>(-1) && c >= 0x20 && c != 0x7f)
            {
                if (c < 0x10000)
                    result += QChar(static_cast<ushort>(c));
                else
                    result += QString::fromUcs4(&c, 1);
            }
        }

        if (absRow < selBot)
            result += QLatin1Char('\n');
    }

    return result;
}

void KTermWidget::copySelection()
{
    QString text = getSelectedText();
    if (!text.isEmpty())
        QApplication::clipboard()->setText(text);
}

void KTermWidget::pasteClipboard()
{
    QString text = QApplication::clipboard()->text();
    if (!text.isEmpty())
        emit sendData(text.toUtf8());
}

void KTermWidget::selectAll()
{
    m_selStart.row = 0;
    m_selStart.col = 0;
    m_selEnd.row = m_scrollBuffer.size() + m_rows - 1;
    m_selEnd.col = m_cols - 1;
    m_hasSelection = true;
    update();
}

void KTermWidget::resetScrollOffset()
{
    if (m_scrollOffset > 0)
    {
        m_scrollOffset = 0;
        update();
    }
}

void KTermWidget::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu menu(this);

    QAction *copyAct = menu.addAction(QString::fromUtf8("\xe5\xa4\x8d\xe5\x88\xb6 (Ctrl+Shift+C)"));
    copyAct->setEnabled(m_hasSelection);
    connect(copyAct, &QAction::triggered, this, &KTermWidget::copySelection);

    QAction *pasteAct = menu.addAction(QString::fromUtf8("\xe7\xb2\x98\xe8\xb4\xb4 (Ctrl+Shift+V)"));
    connect(pasteAct, &QAction::triggered, this, &KTermWidget::pasteClipboard);

    menu.addSeparator();

    QAction *selectAllAct = menu.addAction(QString::fromUtf8("\xe5\x85\xa8\xe9\x80\x89"));
    connect(selectAllAct, &QAction::triggered, this, &KTermWidget::selectAll);

    menu.exec(event->globalPos());
}
