//
// Copyright (C) Wojciech Jarosz <wjarosz@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE.txt file.
//

#include "imagelistpanel.h"
#include "hdrviewer.h"
#include "glimage.h"
#include "hdrimagemanager.h"
#include "imagebutton.h"
#include "hdrimageviewer.h"
#include "multigraph.h"
#include "well.h"
#include <spdlog/spdlog.h>

using namespace std;

ImageListPanel::ImageListPanel(Widget *parent, HDRViewScreen * screen, HDRImageManager * imgMgr, HDRImageViewer * imgViewer)
	: Widget(parent), m_screen(screen), m_imageMgr(imgMgr), m_imageViewer(imgViewer)
{
	setId("image list panel");
	setLayout(new BoxLayout(Orientation::Vertical, Alignment::Fill, 5, 5));

	auto row = new Widget(this);
	row->setLayout(new GridLayout(Orientation::Horizontal, 5, Alignment::Fill, 0, 2));

	auto b = new Button(row, "", ENTYPO_ICON_FOLDER);
	b->setFixedHeight(25);
	b->setTooltip("Load an image and add it to the set of opened images.");
	b->setCallback([this]{m_screen->loadImage();});

	m_saveButton = new Button(row, "", ENTYPO_ICON_SAVE);
	m_saveButton->setEnabled(m_imageMgr->currentImage() != nullptr);
	m_saveButton->setFixedHeight(25);
	m_saveButton->setTooltip("Save the image to disk.");
	m_saveButton->setCallback([this]{m_screen->saveImage();});

	m_bringForwardButton = new Button(row, "", ENTYPO_ICON_UP_BOLD);
	m_bringForwardButton->setFixedHeight(25);
	m_bringForwardButton->setTooltip("Bring the image forward/up the stack.");
	m_bringForwardButton->setCallback([this]{ m_imageMgr->bringImageForward();});

	m_sendBackwardButton = new Button(row, "", ENTYPO_ICON_DOWN_BOLD);
	m_sendBackwardButton->setFixedHeight(25);
	m_sendBackwardButton->setTooltip("Send the image backward/down the stack.");
	m_sendBackwardButton->setCallback([this]{ m_imageMgr->sendImageBackward();});

	m_closeButton = new Button(row, "", ENTYPO_ICON_CIRCLED_CROSS);
	m_closeButton->setFixedHeight(25);
	m_closeButton->setTooltip("Close image");
	m_closeButton->setCallback([this]{m_screen->askCloseImage(m_imageMgr->currentImageIndex());});


	row = new Widget(this);
	row->setLayout(new BoxLayout(Orientation::Vertical,
	                             Alignment::Fill, 0, 4));
	m_graph = new MultiGraph(row, "", Color(255, 0, 0, 150));
	m_graph->addPlot(Color(0, 255, 0, 150));
	m_graph->addPlot(Color(0, 0, 255, 150));

	auto grid = new Widget(this);
	auto agl = new AdvancedGridLayout({0, 4, 19, 4, 0, 4, 0});
	grid->setLayout(agl);
	agl->setColStretch(4, 1.0f);
	agl->setColStretch(6, 1.0f);

	agl->appendRow(0);
	agl->setAnchor(new Label(grid, "Histogram:", "sans", 14), AdvancedGridLayout::Anchor(0, agl->rowCount()-1, Alignment::Fill, Alignment::Fill));

	m_recomputeHistogram = new Button(grid, "", ENTYPO_ICON_WARNING);
	agl->setAnchor(m_recomputeHistogram, AdvancedGridLayout::Anchor(2, agl->rowCount()-1, Alignment::Maximum, Alignment::Fill));

	m_linearToggle = new Button(grid, "Linear");
	agl->setAnchor(m_linearToggle, AdvancedGridLayout::Anchor(4, agl->rowCount()-1, Alignment::Fill, Alignment::Fill));

	m_sRGBToggle = new Button(grid, "sRGB");
	agl->setAnchor(m_sRGBToggle, AdvancedGridLayout::Anchor(6, agl->rowCount()-1, Alignment::Fill, Alignment::Fill));

	m_linearToggle->setFlags(Button::RadioButton);
	m_sRGBToggle->setFlags(Button::RadioButton);
	m_linearToggle->setFixedHeight(19);
	m_sRGBToggle->setFixedHeight(19);
	m_linearToggle->setTooltip("Show sRGB histogram.");
	m_sRGBToggle->setTooltip("Show linear histogram.");
	m_linearToggle->setPushed(true);
	m_linearToggle->setChangeCallback([this](bool b)
	                                  {
		                                  updateHistogram();
	                                  });

	m_recomputeHistogram->setFixedSize(Vector2i(19,19));
	m_recomputeHistogram->setTooltip("Recompute histogram at current exposure.");
	m_recomputeHistogram->setCallback([&]()
	                                  {
		                                  updateHistogram();
	                                  });

	agl->appendRow(4);  // spacing
	agl->appendRow(0);

	agl->setAnchor(new Label(grid, "Mode:", "sans", 14), AdvancedGridLayout::Anchor(0, agl->rowCount()-1, Alignment::Fill, Alignment::Fill));

	m_blendModes = new ComboBox(grid);
	m_blendModes->setItems(blendModeNames());
	m_blendModes->setFixedHeight(19);
	m_blendModes->setCallback([imgViewer](int b){imgViewer->setBlendMode(EBlendMode(b));});
	agl->setAnchor(m_blendModes, AdvancedGridLayout::Anchor(2, agl->rowCount()-1, 5, 1, Alignment::Fill, Alignment::Fill));

	agl->appendRow(4);  // spacing
	agl->appendRow(0);

	agl->setAnchor(new Label(grid, "Channel:", "sans", 14), AdvancedGridLayout::Anchor(0, agl->rowCount()-1, Alignment::Fill, Alignment::Fill));

	m_channels = new ComboBox(grid, channelNames());
	m_channels->setFixedHeight(19);
	setChannel(EChannel::RGB);
	m_channels->setCallback([imgViewer](int c){imgViewer->setChannel(EChannel(c));});
	agl->setAnchor(m_channels, AdvancedGridLayout::Anchor(2, agl->rowCount()-1, 5, 1, Alignment::Fill, Alignment::Fill));
}

EBlendMode ImageListPanel::blendMode() const
{
	return EBlendMode(m_blendModes->selectedIndex());
}

void ImageListPanel::setBlendMode(EBlendMode mode)
{
	m_blendModes->setSelectedIndex(mode);
	m_imageViewer->setBlendMode(mode);
}

EChannel ImageListPanel::channel() const
{
	return EChannel(m_channels->selectedIndex());
}

void ImageListPanel::setChannel(EChannel channel)
{
	m_channels->setSelectedIndex(channel);
	m_imageViewer->setChannel(channel);
}

void ImageListPanel::updateImagesInfo()
{
	if (m_imageMgr->numImages() != (int)m_imageButtons.size())
	{
		spdlog::get("console")->error("Number of buttons and images don't match!");
		return;
	}

	for (int i = 0; i < m_imageMgr->numImages(); ++i)
	{
		auto img = m_imageMgr->image(i);
		auto btn = m_imageButtons[i];

		btn->setIsModified(img->isModified());
		btn->setTooltip(fmt::format("Path: {:s}\n\nResolution: ({:d}, {:d})", img->filename(), img->width(), img->height()));
	}

	m_screen->performLayout();
}


void ImageListPanel::repopulateImageList()
{
	// this currently just clears all the widgets and recreates all of them
	// from scratch. this doesn't scale, but should be fine unless you have a
	// lot of images, and makes the logic a lot simpler.

	// prevent crash when the focus path includes any of the widgets we are destroying
	m_screen->clearFocusPath();
	m_imageButtons.clear();

	// clear everything
	if (m_imageListWidget)
		removeChild(m_imageListWidget);

	m_imageListWidget = new Well(this);
	m_imageListWidget->setLayout(new BoxLayout(Orientation::Vertical, Alignment::Fill, 0));

	for (int i = 0; i < m_imageMgr->numImages(); ++i)
	{
		auto img = m_imageMgr->image(i);
		auto b = new ImageButton(m_imageListWidget, img->filename());
		b->setId(i+1);
		b->setIsModified(img->isModified());
		b->setTooltip(fmt::format("Path: {:s}\n\nResolution: ({:d}, {:d})", img->filename(), img->width(), img->height()));

		b->setSelectedCallback([&](int i){m_imageMgr->setCurrentImageIndex(i-1);});
		b->setReferenceCallback([&](int i){m_imageMgr->setReferenceImageIndex(i-1);});
		m_imageButtons.push_back(b);
	}

	m_screen->performLayout();
}

void ImageListPanel::enableDisableButtons()
{
	m_saveButton->setEnabled(m_imageMgr->currentImage() != nullptr);
	m_closeButton->setEnabled(m_imageMgr->currentImage() != nullptr);
	m_bringForwardButton->setEnabled(m_imageMgr->currentImage() && m_imageMgr->currentImageIndex() > 0);
	m_sendBackwardButton->setEnabled(m_imageMgr->currentImage() && m_imageMgr->currentImageIndex() < m_imageMgr->numImages()-1);
	m_linearToggle->setEnabled(m_imageMgr->currentImage() != nullptr);
	m_sRGBToggle->setEnabled(m_imageMgr->currentImage() != nullptr);
	bool showRecompute = m_imageMgr->currentImage() &&
		(m_imageMgr->currentImage()->histogramDirty() ||
		 m_imageViewer->exposure() != m_imageMgr->currentImage()->histogramExposure());
	m_recomputeHistogram->setEnabled(showRecompute);
	m_recomputeHistogram->setIcon(showRecompute ? ENTYPO_ICON_WARNING : ENTYPO_ICON_CHECK);
//	m_linearToggle->setFixedWidth(showRecompute ? 49 : 59);
//	m_sRGBToggle->setFixedWidth(showRecompute ? 49 : 60);
}

void ImageListPanel::setCurrentImage(int newIndex)
{
	for (int i = 0; i < (int) m_imageButtons.size(); ++i)
		m_imageButtons[i]->setIsSelected(i == newIndex ? true : false);

	updateHistogram();
}

void ImageListPanel::setReferenceImage(int newIndex)
{
	for (int i = 0; i < (int) m_imageButtons.size(); ++i)
		m_imageButtons[i]->setIsReference(i == newIndex ? true : false);
}

void ImageListPanel::draw(NVGcontext *ctx)
{
	// update everything
	//updateHistogram();

	if (m_imageMgr->currentImage() &&
		m_imageMgr->currentImage()->histograms() &&
		m_imageMgr->currentImage()->histograms()->ready())
	{
		auto lazyHist = m_imageMgr->currentImage()->histograms();
		auto hist = m_linearToggle->pushed() ? lazyHist->get()->linearHistogram : lazyHist->get()->sRGBHistogram;
		int numBins = hist.rows();
		float maxValue = hist.block(1,0,numBins-2,3).maxCoeff() * 1.3f;
		m_graph->setValues(hist.col(0)/maxValue, 0);
		m_graph->setValues(hist.col(1)/maxValue, 1);
		m_graph->setValues(hist.col(2)/maxValue, 2);
		m_graph->setMinimum(lazyHist->get()->minimum);
		m_graph->setAverage(lazyHist->get()->average);
		m_graph->setMaximum(lazyHist->get()->maximum);
		m_graph->setLinear(m_linearToggle->pushed());
		m_graph->setDisplayMax(pow(2.f, -lazyHist->get()->exposure));
	}
	enableDisableButtons();

	if (m_imageMgr->numImages() != (int)m_imageButtons.size())
		spdlog::get("console")->error("Number of buttons and images don't match!");
	else
	{
		for (int i = 0; i < m_imageMgr->numImages(); ++i)
		{
			auto img = m_imageMgr->image(i);
			auto btn = m_imageButtons[i];
			btn->setProgress(img->progress());
		}
	}

	Widget::draw(ctx);
}

void ImageListPanel::updateHistogram()
{
	if (!m_imageMgr->currentImage())
	{
		m_graph->setValues(VectorXf(), 0);
		m_graph->setValues(VectorXf(), 1);
		m_graph->setValues(VectorXf(), 2);
		enableDisableButtons();
		return;
	}

	m_imageMgr->currentImage()->recomputeHistograms(m_imageViewer->exposure());
	enableDisableButtons();
}