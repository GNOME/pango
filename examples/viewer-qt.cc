/* Pango
 * viewer-qt.cc: Example program to view a UTF-8 encoding file
 *               using Pango to render result.
 *
 * Copyright (C) 1999-2000 Red Hat Software
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <qapplication.h>
#include <qcombobox.h>
#include <qfile.h>
#include <qfileinfo.h>
#include <qmenubar.h>
#include <qpainter.h>
#include <qpopupmenu.h>
#include <qspinbox.h>
#include <qscrollview.h>
#include <qstatusbar.h>
#include <qtextcodec.h>
#include <qtextstream.h>
#include <qtoolbar.h>

#include <pango/pango.h>
#include <pango/pangox.h>

#include "viewer-qt.h"

ViewerPara::ViewerPara (PangoContext *context, const QString &text)
{
  text_ = text.utf8();
  
  layout_ = pango_layout_new (context);
  pango_layout_set_text (layout_, (char *)(const char *)text_, text_.length());
  height_ = -1;
}

ViewerPara::~ViewerPara ()
{
  g_object_unref (G_OBJECT (layout_));
}

void
ViewerPara::setWidth (int width)
{
  pango_layout_set_width (layout_, width * PANGO_SCALE);
  height_ = -1;
}

void
ViewerPara::contextChanged ()
{
  pango_layout_context_changed (layout_);
  height_ = -1;

  PangoContext *context;
  PangoAlignment align;
  
  context = pango_layout_get_context (layout_);
  if (pango_context_get_base_dir (context) == PANGO_DIRECTION_LTR)
    align = PANGO_ALIGN_LEFT;
  else
    align = PANGO_ALIGN_RIGHT;

  pango_layout_set_alignment (layout_, align);
}

int 
ViewerPara::height ()
{
  if (height_ < 0)
    {
      PangoRectangle logical_rect;
      
      pango_layout_get_extents (layout_, NULL, &logical_rect);
      height_ = logical_rect.height / PANGO_SCALE;
    }

  return height_;
}

void 
ViewerPara::draw (QPainter *painter, GC gc, int y)
{
  QPoint devicePt = painter->xForm (QPoint (0, y));
	  
  pango_x_render_layout (painter->device()->x11Display(),
			 painter->device()->handle(),
			 gc, layout_, 0, devicePt.y());
}

gunichar 
ViewerPara::getChar (int index)
{
  gunichar result;

  return g_utf8_get_char ((const char *)text_ + index);
}

QRect 
ViewerPara::charBounds (int index)
{
  PangoRectangle pos;

  pango_layout_index_to_pos (layout_, index, &pos);
  
  return QRect (MIN (pos.x, pos.x + pos.width) / PANGO_SCALE,
		pos.y / PANGO_SCALE,
		ABS (pos.width) / PANGO_SCALE,
		pos.height / PANGO_SCALE);
}

int 
ViewerPara::findPoint (int x, int y)
{
  int index;

  bool result = pango_layout_xy_to_index (layout_,
					  x * PANGO_SCALE, y * PANGO_SCALE,
					  &index, NULL);
  if (result)
    return index;
  else
    return -1;
}

ViewerView::ViewerView (QWidget *parent, QStatusBar *status) : QScrollView (parent)
{
  status_ = status;
  
  viewport()->setBackgroundMode( QWidget::PaletteBase );
  
  setHScrollBarMode (QScrollView::AlwaysOff);

  context_ = pango_x_get_context (x11Display());
  pango_context_set_language (context_, pango_language_from_string ("en-us"));

  highlight_para_ = NULL;
  highlight_index_ = 0;
}

ViewerView::~ViewerView ()
{
  g_object_unref (G_OBJECT (context_));
}

void
ViewerView::readFile (const QString &name)
{
  QFile file (name);
  
  if (!file.open (IO_ReadOnly))
    {
      fprintf (stderr, "Cannot open file '%s'\n", (const char *)name.local8Bit());
      return;
    }
  
  QTextStream ts (&file);;

  ts.setCodec (QTextCodec::codecForName ("utf8"));

  while (!ts.atEnd())
    paragraphs_.append (new ViewerPara (context (), ts.readLine()));
}

void
ViewerView::contextChanged ()
{
  QListIterator<ViewerPara> it(paragraphs_);
  for (; it.current(); ++it)
    it.current()->contextChanged();

  computeHeight();
  updateContents (0, 0, contentsWidth(), contentsHeight());
}

void
ViewerView::setFontDesc (PangoFontDescription *font_desc)
{
  pango_context_set_font_description (context_, font_desc);
  contextChanged();
}

void
ViewerView::setDirection (PangoDirection base_dir)
{
  QListIterator<ViewerPara> it(paragraphs_);
  for (; it.current(); ++it)
    it.current()->contextChanged();

  pango_context_set_base_dir (context_, base_dir);
  contextChanged();
}

void 
ViewerView::drawContents (QPainter *p, int clipx, int clipy, int clipw, int cliph)
{
  XGCValues values;
  QRect clip = p->xForm (QRect (clipx, clipy, clipw, cliph));

  GC gc = XCreateGC (x11Display(), handle(), 0, &values);

  XSetForeground (x11Display(), gc, colorGroup().text().pixel());
  
  XRectangle xclip = { clip.x(), clip.y(), clip.width(), clip.height() };
  XSetClipRectangles (x11Display(), gc, 0, 0, &xclip, 1, YXBanded);

  int y = 0;
  QListIterator<ViewerPara> it(paragraphs_);
  for (; it.current(); ++it)
    {
      ViewerPara *para = it.current();
      int y_end = y + para->height ();

      if (y_end > clipy && y < clipy + cliph)
	para->draw (p, gc, y);

      if (para == highlight_para_)
	{
	  QRect bounds = highlight_para_->charBounds (highlight_index_);
	  bounds.moveBy (0, y);
	  bounds = p->xForm (bounds);

	  XRectangle xbounds = { bounds.x(), bounds.y(), bounds.width(), bounds.height() };

	  XFillRectangle (x11Display(), p->device()->handle(), gc,
			  xbounds.x, xbounds.y, xbounds.width, xbounds.height);

	  XSetClipRectangles (x11Display(), gc, 0, 0, &xbounds, 1, YXBanded);
	  XSetForeground (x11Display(), gc, colorGroup().base().pixel());
	  
	  para->draw (p, gc, y);

	  XSetForeground (x11Display(), gc, colorGroup().text().pixel());
	  XSetClipRectangles (x11Display(), gc, 0, 0, &xclip, 1, YXBanded);
	}
      
      y = y_end;
    }

  XFreeGC (x11Display(), gc);
}

void
ViewerView::computeHeight ()
{
  int width = visibleWidth();
  int height = 0;

  QListIterator<ViewerPara> it(paragraphs_);
  for (; it.current(); ++it)
    {
      ViewerPara *para = it.current();
      para->setWidth (width);
      height += para->height ();
    }

  resizeContents (width, height);
  repaintContents (0, 0, contentsWidth(), contentsHeight());
}

void 
ViewerView::viewportResizeEvent (QResizeEvent *event)
{
  computeHeight ();
}

void
ViewerView::updateHighlightChar ()
{
  if (highlight_para_)
    {
      int y = 0;
      QListIterator<ViewerPara> it(paragraphs_);
      
      for (; it.current(); ++it)
	{
	  ViewerPara *para = it.current();
	  if (para == highlight_para_)
	    {
	      QRect bounds = para->charBounds (highlight_index_);
	      bounds.moveBy (0, y);
	      updateContents (bounds.x(), bounds.y(), bounds.width(), bounds.height());
	      
	      break;
	    }
	  
	  y += para->height ();
	}
    }
}

void
ViewerView::contentsMousePressEvent (QMouseEvent *event)
{
  updateHighlightChar ();
  highlight_para_ = NULL;
  
  int y = 0;
  QListIterator<ViewerPara> it(paragraphs_);
  for (; it.current(); ++it)
    {
      ViewerPara *para = it.current();
      int y_end = y + para->height ();

      if (y <= event->y() && event->y() < y_end)
	{
	  int index = para->findPoint (event->x(), event->y() - y);
	  gunichar wc = para->getChar (index);

	  if (index >= 0 && wc != (gunichar)-1)
	    {
	      highlight_para_ = para;
	      highlight_index_ = index;
	      
	      if (status_)
		{
		  QString num = QString::number (wc, 16).rightJustify (4, '0');
		  
		  status_->message (QString ("Current char: U+") + num);
		}
	    }
	  
	  break;
	}
      y = y_end;
    }

  if (status_ && !highlight_para_)
    status_->clear ();

  updateHighlightChar();
}

static int
cmp_families (const void *a, const void *b)
{
  const char *a_name = pango_font_family_get_name (*(PangoFontFamily **)a);
  const char *b_name = pango_font_family_get_name (*(PangoFontFamily **)b);
  
  return strcmp (a_name, b_name);
}

ViewerWindow::ViewerWindow (const QString &filename)
{
  // Create menu

  file_menu_ = new QPopupMenu (this);
  file_menu_->setCheckable (true);
  
  menuBar()->insertItem ("&File", file_menu_);

  file_menu_->insertItem( "&Quit", qApp, SLOT( closeAllWindows() ), CTRL+Key_Q );

  right_to_left_item_ = file_menu_->insertItem( "&Right-to-Left", this, SLOT( setRightToLeft() ), 0 );

  view_ = new ViewerView (this, new QStatusBar (this));
  
  setCentralWidget (view_);

  // Create families combo box

  QToolBar *toolbar = new QToolBar ("Font", this);

  family_combo_ = new QComboBox (toolbar);

  int n_families, i;

  pango_context_list_families (view_->context(), &families_, &n_families);
  qsort (families_, n_families, sizeof(PangoFontFamily *), cmp_families);

  for (i=0; i<n_families; i++)
    {
      const char *name = pango_font_family_get_name (families_[i]);
      
      family_combo_->insertItem (name);
      if (!strcmp (name, "sans"))
	family_combo_->setCurrentItem (i);
    }

  QObject::connect (family_combo_, SIGNAL(activated(int)), this, SLOT(fillStyleCombo(int)));

  faces_ = NULL;

  style_combo_ = new QComboBox (toolbar);
  fillStyleCombo (family_combo_->currentItem ());

  size_box_ = new QSpinBox (1, 1000, 1, toolbar);
  size_box_->setValue (16);

  QObject::connect (family_combo_, SIGNAL(activated(int)), this, SLOT(fontChanged()));
  QObject::connect (style_combo_, SIGNAL(activated(int)), this, SLOT(fontChanged()));
  QObject::connect (size_box_, SIGNAL(valueChanged(int)), this, SLOT(fontChanged()));

  fontChanged ();

  view_->readFile (filename);

  resize (500, 500);
}

static int
compare_font_descriptions (const PangoFontDescription *a, const PangoFontDescription *b)
{
  int val = strcmp (pango_font_description_get_family (a), pango_font_description_get_family (b));
  if (val != 0)
    return val;

  if (pango_font_description_get_weight (a) != pango_font_description_get_weight (b))
    return pango_font_description_get_weight (a) - pango_font_description_get_weight (b);

  if (pango_font_description_get_style (a) != pango_font_description_get_style (b))
    return pango_font_description_get_style (a) - pango_font_description_get_style (b);
  
  if (pango_font_description_get_stretch (a) != pango_font_description_get_stretch (b))
    return pango_font_description_get_stretch (a) - pango_font_description_get_stretch (b);

  if (pango_font_description_get_variant (a) != pango_font_description_get_variant (b))
    return pango_font_description_get_variant (a) - pango_font_description_get_variant (b);

  return 0;
}

static int
faces_sort_func (const void *a, const void *b)
{
  PangoFontDescription *desc_a = pango_font_face_describe (*(PangoFontFace **)a);
  PangoFontDescription *desc_b = pango_font_face_describe (*(PangoFontFace **)b);
  
  int ord = compare_font_descriptions (desc_a, desc_b);

  pango_font_description_free (desc_a);
  pango_font_description_free (desc_b);

  return ord;
}

void
ViewerWindow::fillStyleCombo (int index)
{
  PangoFontFamily *family = families_[index];
  int n_faces;

  if (faces_)
    g_free (faces_);

  pango_font_family_list_faces (family, &faces_, &n_faces);

  qsort (faces_, n_faces, sizeof(PangoFontFace *), faces_sort_func);

  style_combo_->clear();
  for (int i = 0; i < n_faces; i++)
    {
      const char *str = pango_font_face_get_face_name (faces_[i]);
      style_combo_->insertItem (str);
    }
}

void
ViewerWindow::fontChanged ()
{
  PangoFontFace *face = faces_[style_combo_->currentItem()];

  QString style = style_combo_->currentText();

  PangoFontDescription *font_desc = pango_font_face_describe (face);
  pango_font_description_set_size (font_desc, size_box_->value () * PANGO_SCALE);

  view_->setFontDesc (font_desc);

  pango_font_description_free (font_desc);
}

void 
ViewerWindow::setRightToLeft ()
{
  bool new_value = !file_menu_->isItemChecked (right_to_left_item_);
  
  file_menu_->setItemChecked (right_to_left_item_, new_value);
  view_->setDirection (new_value ? PANGO_DIRECTION_RTL : PANGO_DIRECTION_LTR);
}

ViewerWindow::~ViewerWindow ()
{
  delete view_;
}

int 
main (int argc, char **argv)
{
  QApplication a (argc, argv);
  const char *filename;

  g_type_init ();
  
  if (QFileInfo ("./pangorc").exists ())
    putenv ("PANGO_RC_FILE=./pangorc");

  if (argc == 2)
    filename = argv[1];
  else
    filename = "HELLO.utf8";
    
  ViewerWindow *vw = new ViewerWindow (QString::fromLocal8Bit (filename));

  vw->show();

  a.connect (&a, SIGNAL(lastWindowClosed()), &a, SLOT(quit()));
  return a.exec ();
}

