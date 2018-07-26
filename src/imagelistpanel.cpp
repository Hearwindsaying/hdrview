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

	// histogram mode selection GUI elements
	{
		auto grid = new Widget(this);
		auto agl = new AdvancedGridLayout({0, 4, 0, 4, 0});
		grid->setLayout(agl);
		agl->setColStretch(2, 1.0f);
		agl->setColStretch(4, 1.0f);

		agl->appendRow(0);
		agl->setAnchor(new Label(grid, "Histogram:", "sans", 14),
		               AdvancedGridLayout::Anchor(0, agl->rowCount() - 1, Alignment::Fill, Alignment::Fill));

		m_yAxisScale = new ComboBox(grid);
		m_yAxisScale->setTooltip("Set the scale for the Y axis.");
		m_yAxisScale->setItems({"Linear", "Log"});
		m_yAxisScale->setFixedHeight(19);
		agl->setAnchor(m_yAxisScale,
		               AdvancedGridLayout::Anchor(2, agl->rowCount() - 1, 1, 1, Alignment::Fill, Alignment::Fill));

		m_xAxisScale = new ComboBox(grid);
		m_xAxisScale->setTooltip("Set the scale for the X axis.");
		m_xAxisScale->setItems({"Linear", "sRGB", "Log"});
		m_xAxisScale->setFixedHeight(19);
		agl->setAnchor(m_xAxisScale,
		               AdvancedGridLayout::Anchor(4, agl->rowCount() - 1, 1, 1, Alignment::Fill, Alignment::Fill));

		m_xAxisScale->setSelectedIndex(1);
		m_yAxisScale->setSelectedIndex(0);
		m_xAxisScale->setCallback([this](int) { updateHistogram(); });
		m_yAxisScale->setCallback([this](int) { updateHistogram(); });
	}

	// histogram and file buttons
	{
		auto row = new Widget(this);
		row->setLayout(new BoxLayout(Orientation::Vertical,
		                             Alignment::Fill, 0, 4));
		m_graph = new MultiGraph(row, Color(255, 0, 0, 150));
		m_graph->addPlot(Color(0, 255, 0, 150));
		m_graph->addPlot(Color(0, 0, 255, 150));

		row = new Widget(this);
		row->setLayout(new GridLayout(Orientation::Horizontal, 5, Alignment::Fill, 0, 2));

		auto b = new Button(row, "", ENTYPO_ICON_FOLDER);
		b->setFixedHeight(25);
		b->setTooltip("Load an image and add it to the set of opened images.");
		b->setCallback([this] { m_screen->loadImage(); });

		m_saveButton = new Button(row, "", ENTYPO_ICON_SAVE);
		m_saveButton->setEnabled(m_imageMgr->currentImage() != nullptr);
		m_saveButton->setFixedHeight(25);
		m_saveButton->setTooltip("Save the image to disk.");
		m_saveButton->setCallback([this] { m_screen->saveImage(); });

		m_bringForwardButton = new Button(row, "", ENTYPO_ICON_ARROW_BOLD_UP);
		m_bringForwardButton->setFixedHeight(25);
		m_bringForwardButton->setTooltip("Bring the image forward/up the stack.");
		m_bringForwardButton->setCallback([this]
		                                  {
			                                  if (m_imageMgr->bringImageForward())
			                                  {
				                                  m_imageButtons[m_imageMgr->currentImageIndex()]
					                                  ->recomputeStringClipping();
				                                  m_imageButtons[m_imageMgr->currentImageIndex() + 1]
					                                  ->recomputeStringClipping();
			                                  }
		                                  });

		m_sendBackwardButton = new Button(row, "", ENTYPO_ICON_ARROW_BOLD_DOWN);
		m_sendBackwardButton->setFixedHeight(25);
		m_sendBackwardButton->setTooltip("Send the image backward/down the stack.");
		m_sendBackwardButton->setCallback([this]
		                                  {
			                                  if (m_imageMgr->sendImageBackward())
			                                  {
				                                  m_imageButtons[m_imageMgr->currentImageIndex()]
					                                  ->recomputeStringClipping();
				                                  m_imageButtons[m_imageMgr->currentImageIndex() - 1]
					                                  ->recomputeStringClipping();
			                                  }
		                                  });

		m_closeButton = new Button(row, "", ENTYPO_ICON_CIRCLE_WITH_CROSS);
		m_closeButton->setFixedHeight(25);
		m_closeButton->setTooltip("Close image");
		m_closeButton->setCallback([this] { m_screen->askCloseImage(m_imageMgr->currentImageIndex()); });
	}

	// channel and blend mode GUI elements
	{
		auto grid = new Widget(this);
		auto agl = new AdvancedGridLayout({0, 4, 0});
		grid->setLayout(agl);
		agl->setColStretch(2, 1.0f);

		agl->appendRow(0);
		agl->setAnchor(new Label(grid, "Mode:", "sans", 14),
		               AdvancedGridLayout::Anchor(0, agl->rowCount() - 1, Alignment::Fill, Alignment::Fill));

		m_blendModes = new ComboBox(grid);
		m_blendModes->setItems(blendModeNames());
		m_blendModes->setFixedHeight(19);
		m_blendModes->setCallback([imgViewer](int b) { imgViewer->setBlendMode(EBlendMode(b)); });
		agl->setAnchor(m_blendModes,
		               AdvancedGridLayout::Anchor(2, agl->rowCount() - 1, Alignment::Fill, Alignment::Fill));

		agl->appendRow(4);  // spacing
		agl->appendRow(0);

		agl->setAnchor(new Label(grid, "Channel:", "sans", 14),
		               AdvancedGridLayout::Anchor(0, agl->rowCount() - 1, Alignment::Fill, Alignment::Fill));

		m_channels = new ComboBox(grid, channelNames());
		m_channels->setFixedHeight(19);
		setChannel(EChannel::RGB);
		m_channels->setCallback([imgViewer](int c) { imgViewer->setChannel(EChannel(c)); });
		agl->setAnchor(m_channels,
		               AdvancedGridLayout::Anchor(2, agl->rowCount() - 1, Alignment::Fill, Alignment::Fill));
	}

	// filter/search of open images GUI elemen ts
	{
		auto grid = new Widget(this);
		auto agl = new AdvancedGridLayout({0, 4, 0});
		grid->setLayout(agl);
		agl->setColStretch(0, 1.0f);

		agl->appendRow(0);

		m_filter = new TextBox(grid, "");
		m_filter->setEditable(true);
		m_filter->setAlignment(TextBox::Alignment::Left);
//		m_filter->setCallback([this](const string& filter) {
//			return setFilter(filter);
//		});

		m_filter->setPlaceholder("Find");
		m_filter->setTooltip("Filter open image list so that only images with a filename containing the search string will be visible.");

		agl->setAnchor(m_filter,
		               AdvancedGridLayout::Anchor(0, agl->rowCount() - 1, Alignment::Fill, Alignment::Fill));


		m_useShortButton = new Button(grid, "", ENTYPO_ICON_FUNNEL);
		m_useShortButton->setFixedWidth(19);
		m_useShortButton->setFixedHeight(19);
		m_useShortButton->setTooltip("Toggle showing full filenames vs. only the unique portion of each filename.");
		m_useShortButton->setFlags(Button::ToggleButton);
		m_useShortButton->setPushed(false);
		m_useShortButton->setChangeCallback([this](bool b)
		                                    {
			                                    for (auto ib : m_imageButtons)
				                                    ib->setUseShortCaption(b);
		                                    });

		agl->setAnchor(m_useShortButton,
		               AdvancedGridLayout::Anchor(2, agl->rowCount() - 1, Alignment::Minimum, Alignment::Fill));

	}
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

	recomputeShortFilenames();

	m_screen->performLayout();
}

void ImageListPanel::recomputeShortFilenames()
{
	vector<string> activeImageNames;
	for (int i = 0; i < m_imageMgr->numImages(); ++i)
	{
		auto img = m_imageMgr->image(i);
		activeImageNames.emplace_back(img->filename());
	}

	// determine common parts of filenames
	// taken from tev
	m_beginShortOffset = 0;
	m_endShortOffset = 0;
	if (!activeImageNames.empty())
	{
		string first = activeImageNames.front();
		int firstSize = (int)first.size();
		if (firstSize > 0)
		{
			bool allStartWithSameChar = false;
			do
			{
				int len = codePointLength(first[m_beginShortOffset]);

				allStartWithSameChar = all_of
					(
						begin(activeImageNames),
						end(activeImageNames),
						[&first, this, len](const string& name)
						{
							if (m_beginShortOffset + len > (int)name.size())
								return false;

							for (int i = m_beginShortOffset; i < m_beginShortOffset + len; ++i)
								if (name[i] != first[i])
									return false;

							return true;
						}
					);

				if (allStartWithSameChar)
					m_beginShortOffset += len;
			}
			while (allStartWithSameChar && m_beginShortOffset < firstSize);

			bool allEndWithSameChar;
			do
			{
				char lastChar = first[firstSize - m_endShortOffset - 1];
				allEndWithSameChar = all_of
					(
						begin(activeImageNames),
						end(activeImageNames),
						[lastChar, this](const string& name)
						{
							int index = (int)name.size() - m_endShortOffset - 1;
							return index >= 0 && name[index] == lastChar;
						}
					);

				if (allEndWithSameChar)
					++m_endShortOffset;
			}
			while (allEndWithSameChar && m_endShortOffset < firstSize);
		}
	}

	for (auto ib : m_imageButtons)
	{
		ib->setUseShortCaption(m_useShortButton->pushed());
		ib->setHighlightRange(m_beginShortOffset, m_endShortOffset);
	}
}

void ImageListPanel::enableDisableButtons()
{
	bool hasImage = m_imageMgr->currentImage() != nullptr;
	bool hasValidImage = hasImage && !m_imageMgr->currentImage()->isNull();
	m_saveButton->setEnabled(hasValidImage);
	m_closeButton->setEnabled(hasImage);
	m_bringForwardButton->setEnabled(hasImage && m_imageMgr->currentImageIndex() > 0);
	m_sendBackwardButton->setEnabled(hasImage && m_imageMgr->currentImageIndex() < m_imageMgr->numImages()-1);
}

void ImageListPanel::setCurrentImage(int newIndex)
{
	for (int i = 0; i < (int) m_imageButtons.size(); ++i)
		m_imageButtons[i]->setIsSelected(i == newIndex ? true : false);

	requestHistogramUpdate(true);
}

void ImageListPanel::setReferenceImage(int newIndex)
{
	for (int i = 0; i < (int) m_imageButtons.size(); ++i)
		m_imageButtons[i]->setIsReference(i == newIndex ? true : false);
}

void ImageListPanel::draw(NVGcontext *ctx)
{
	// if it has been more than 2 seconds since we requested a histogram update, then update it
	if (m_histogramUpdateRequested &&
		(glfwGetTime() - m_histogramRequestTime) > 1.0)
		updateHistogram();

	if (m_histogramDirty &&
		m_imageMgr->currentImage() &&
		!m_imageMgr->currentImage()->isNull() &&
		m_imageMgr->currentImage()->histograms() &&
		m_imageMgr->currentImage()->histograms()->ready())
	{
		auto lazyHist = m_imageMgr->currentImage()->histograms();
		int idx = m_xAxisScale->selectedIndex();
		int idxY = m_yAxisScale->selectedIndex();
		auto hist = lazyHist->get()->histogram[idx].values;
		auto ticks = lazyHist->get()->histogram[idx].xTicks;
		auto labels = lazyHist->get()->histogram[idx].xTickLabels;
		m_graph->setValues(idxY == 0 ? hist.col(0) : hist.col(0).unaryExpr([](float v){return normalizedLogScale(v);}).eval(), 0);
		m_graph->setValues(idxY == 0 ? hist.col(1) : hist.col(1).unaryExpr([](float v){return normalizedLogScale(v);}).eval(), 1);
		m_graph->setValues(idxY == 0 ? hist.col(2) : hist.col(2).unaryExpr([](float v){return normalizedLogScale(v);}).eval(), 2);
		m_graph->setXTicks(ticks, labels);
		VectorXf yTicks = VectorXf::LinSpaced(9,0.0f,1.0f);
		if (idxY != 0) yTicks = yTicks.unaryExpr([](float v){return normalizedLogScale(v);});
		m_graph->setYTicks(yTicks);
		m_graph->setLeftHeader(fmt::format("{:.3f}", lazyHist->get()->minimum));
		m_graph->setCenterHeader(fmt::format("{:.3f}", lazyHist->get()->average));
		m_graph->setRightHeader(fmt::format("{:.3f}", lazyHist->get()->maximum));
		m_histogramDirty = false;
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
			btn->setIsModified(img->isModified());
			btn->setCaption(img->filename());
			btn->setHighlightRange(m_beginShortOffset, m_endShortOffset);
			btn->setTooltip(fmt::format("Path: {:s}\n\nResolution: ({:d}, {:d})", img->filename(), img->width(), img->height()));
		}
	}

	Widget::draw(ctx);
}

void ImageListPanel::updateHistogram()
{
	m_histogramDirty = true;

	m_graph->setValues(VectorXf(), 0);
	m_graph->setValues(VectorXf(), 1);
	m_graph->setValues(VectorXf(), 2);

	m_graph->setLeftHeader("");
	m_graph->setCenterHeader("");
	m_graph->setRightHeader("");

	m_graph->setXTicks(VectorXf(), {});
	m_graph->setYTicks(VectorXf());

	if (m_imageMgr->currentImage())
		m_imageMgr->currentImage()->recomputeHistograms(m_imageViewer->exposure());

	m_histogramUpdateRequested = false;
	m_histogramRequestTime = glfwGetTime();
}

void ImageListPanel::requestHistogramUpdate(bool force)
{
	if (force)
		updateHistogram();
	else// if (!m_histogramUpdateRequested)
	{
		// if no histogram update is pending, then queue one up, and start the timer
		m_histogramUpdateRequested = true;
		m_histogramRequestTime = glfwGetTime();
	}
}