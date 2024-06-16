#include "pch.h"
#include "App.xaml.h"

using namespace std;
using namespace winrt;
using namespace Windows::Foundation;
using namespace Windows::System;
using namespace Windows::UI::Core;
using namespace Microsoft::UI::Input;
using namespace Microsoft::UI::Windowing;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Media::Imaging;

namespace winrt::wfb::implementation
{
	App::App()
	{
	}

	void App::PositionWindowPercentage(Window const& window, double px, double py)
	{
		auto width = displayBounds.right - displayBounds.left;
		auto height = displayBounds.bottom - displayBounds.top;

		auto x = displayBounds.left + static_cast<int>(width * px);
		auto y = displayBounds.top + static_cast<int>(height * py);

		window.AppWindow().Move({ x, y });
	}

	void App::SizeWindowPercentage(Window const& window, double pw, double ph)
	{
		auto width = displayBounds.right - displayBounds.left;
		auto height = displayBounds.bottom - displayBounds.top;

		auto w = static_cast<int>(width * pw);
		auto h = static_cast<int>(height * ph);

		window.AppWindow().Resize({ w, h });
	}

	void App::SetupWindowPresenter(const winrt::Microsoft::UI::Xaml::Window& window)
	{
		auto presenter = window.AppWindow().Presenter().as<OverlappedPresenter>();
		presenter.IsResizable(false);
		presenter.IsAlwaysOnTop(true);
		presenter.IsMaximizable(false);
		presenter.IsMinimizable(false);
	}

	void App::CreateScoreWindow()
	{
		if (scoreWindow)
		{
			UpdateScore();
			return;
		}

		scoreText = {};
		scoreText.HorizontalAlignment(HorizontalAlignment::Center);
		scoreText.VerticalAlignment(VerticalAlignment::Center);

		scoreWindow = {};
		SetupWindowPresenter(scoreWindow);
		scoreWindow.Content(scoreText);
		scoreWindow.Title(L"Score");
		scoreWindow.Activate();

		UpdateScore();

		HandleWindowClose(scoreWindow);
		PositionWindowPercentage(scoreWindow, 0.05, 0.05);
		SizeWindowPercentage(scoreWindow, 0.05, 0.05);
	}

	void App::CreateBirdWindow()
	{
		Grid grid;
		grid.RenderTransformOrigin({ 0.5, 0.5 });
		grid.RenderTransform(birdRotateTransform);

		birdImages.clear();
		for (int i = 0; i < 4; ++i)
		{
			birdImages.push_back({});
			birdImages[i].Source(BitmapImage{ Uri{L"ms-appx:///Assets/bird" + to_hstring(i) + L".png"} });
			birdImages[i].Visibility(Visibility::Collapsed);
			grid.Children().Append(birdImages[i]);
		}

		birdWindow = {};
		SetupWindowPresenter(birdWindow);
		birdWindow.Content(grid);
		birdWindow.Title(L"Bird");
		birdWindow.Activate();

		HandleWindowClose(birdWindow);
		SizeWindowPercentage(birdWindow, pBirdW, pBirdH);
		UpdateBirdPosition();

		// flap the bird
		flapTimer = {};
		flapTimer.Interval(std::chrono::milliseconds(333));
		flapTimer.Tick([this](auto&&, auto&&) { UpdateBirdFrame(); });
		flapTimer.Start();
	}

	void App::UpdateBirdFrame()
	{
		birdImages[birdFrame].Visibility(Visibility::Collapsed);
		++birdFrame %= birdImages.size();
		birdImages[birdFrame].Visibility(Visibility::Visible);

		birdRotateTransform.Angle(pBirdVY >= 0 ? 20 : -20);
	}

	void App::CreatePipeWindows(double px)
	{
		auto pGapH = rand() / (double)RAND_MAX * (1 - pPipeGapH - 0.4) + 0.2;

		for (int i = 0; i < 2; ++i)
		{
			Image pipeImage;
			pipeImage.Stretch(Stretch::Fill);
			pipeImage.Source(BitmapImage{ Uri{L"ms-appx:///Assets/pipe.png"} });
			if (i)
			{
				pipeImage.RenderTransformOrigin({ 0.5, 0.5 });
				RotateTransform rotateTransform;
				rotateTransform.Angle(180);
				pipeImage.RenderTransform(rotateTransform);
			}

			auto pipeWindow = freePipeWindows.empty() ? Window{} : freePipeWindows.back();
			if (!freePipeWindows.empty()) freePipeWindows.pop_back();
			pipeWindow.Title(L"Pipe");
			pipeWindow.Content(pipeImage);
			SetupWindowPresenter(pipeWindow);
			pipeWindow.Activate();
			pipeWindows.push_back(pipeWindow);

			pPipeXs.push_back(px);
			pPipeYs.push_back(i == 0 ? 0 : pGapH + pPipeGapH);
			pPipeHs.push_back(i == 0 ? pGapH : 1 - pGapH - pPipeGapH);

			HandleWindowClose(pipeWindow);
			PositionWindowPercentage(pipeWindow, px, pPipeYs.back());
			SizeWindowPercentage(pipeWindow, pPipeW, pPipeHs.back());
		}
	}

	void App::HandleWindowClose(const Window& window)
	{
		window.Closed([this](auto&&, auto&&) { if (!allowWindowClose) Exit(); });
	}

	void App::StartGame()
	{
		pBirdX = 0.1;
		pBirdY = 0.5;
		pBirdVY = 0;
		pipeWindows.clear();
		pPipeXs.clear();
		pPipeYs.clear();
		pPipeHs.clear();
		score = 0;

		CreateScoreWindow();
		CreateBirdWindow();

		CreatePipeWindows(0.5);
		CreatePipeWindows(1);

		// logic loop
		logicTimer = {};
		logicTimer.Interval(std::chrono::milliseconds(33));
		logicTimer.Tick([this](auto&&, auto&&)
			{
				if ((InputKeyboardSource::GetKeyStateForCurrentThread(VirtualKey::Escape) & CoreVirtualKeyStates::Down) == CoreVirtualKeyStates::Down)
					Exit();

				if (DateTime::clock::now() - lastBirdJumpTime > std::chrono::milliseconds(400)
					&& (InputKeyboardSource::GetKeyStateForCurrentThread(VirtualKey::Space) & CoreVirtualKeyStates::Down) == CoreVirtualKeyStates::Down)
				{
					pBirdVY = -0.015;
					lastBirdJumpTime = DateTime::clock::now();
				}

				pBirdY += pBirdVY;
				pBirdVY += pBirdAY;
				UpdateBirdPosition();

				if (pBirdY - pBirdH / 2 < 0 || pBirdY + pBirdH / 2 > 1)
				{
					GameOver();
					return;
				}

				// pipes advance
				for (auto&& pipeWindow : pipeWindows)
				{
					auto i = &pipeWindow - &pipeWindows[0];
					pPipeXs[i] += pPipeVX;
					PositionWindowPercentage(pipeWindow, pPipeXs[i], pPipeYs[i]);

					if (pBirdX + pBirdW >= pPipeXs[i] && pBirdX - pBirdW <= pPipeXs[i] + pPipeW
						&& pBirdY - pBirdH / 2 >= pPipeYs[i] && pBirdY + pBirdH / 2 <= pPipeYs[i] + pPipeHs[i])
					{
						GameOver();
						return;
					}
				}

				// passed any pipes?
				vector<int> erasedPipeIndices;
				for (auto&& pipeWindow : pipeWindows)
				{
					auto i = &pipeWindow - &pipeWindows[0];
					if (pBirdX > pPipeXs[i] + pPipeW)
						erasedPipeIndices.push_back(i);
				}

				reverse(erasedPipeIndices.begin(), erasedPipeIndices.end());
				if (!erasedPipeIndices.empty())
				{
					for (auto&& i : erasedPipeIndices)
					{
						//allowWindowClose = true;
						//pipeWindows[i].Close();
						//allowWindowClose = false;
						freePipeWindows.push_back(pipeWindows[i]);
						pipeWindows.erase(pipeWindows.begin() + i);
						pPipeXs.erase(pPipeXs.begin() + i);
						pPipeYs.erase(pPipeYs.begin() + i);
						pPipeHs.erase(pPipeHs.begin() + i);
					}

					++score;
					UpdateScore();

					CreatePipeWindows(1);
				}
			});
		logicTimer.Start();
	}

	void App::GameOver()
	{
		flapTimer.Stop();
		logicTimer.Stop();

		allowWindowClose = true;
		birdWindow.Close();
		for (auto&& pipeWindow : pipeWindows)
			pipeWindow.Close();
		pipeWindows.clear();
		for (auto&& pipeWindow : freePipeWindows)
			pipeWindow.Close();
		freePipeWindows.clear();
		allowWindowClose = false;

		Window gameOverWindow;
		TextBlock gameOverText;
		gameOverText.Text(L"Game Over\n\nYour score: " + to_hstring(score));
		gameOverText.HorizontalAlignment(HorizontalAlignment::Center);
		gameOverText.VerticalAlignment(VerticalAlignment::Center);
		gameOverWindow.Content(gameOverText);
		gameOverWindow.Title(L"Game Over");
		gameOverWindow.Activate();

		PositionWindowPercentage(gameOverWindow, 0.4, 0.4);
		SizeWindowPercentage(gameOverWindow, 0.2, 0.2);

		gameOverWindow.Closed([this](auto&&, auto&&) { StartGame(); });
	}

	static BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, [[maybe_unused]] HDC hdcMonitor, [[maybe_unused]] LPRECT lprcMonitor, LPARAM dwData)
	{
		auto displayBounds = reinterpret_cast<RECT*>(dwData);

		MONITORINFOEX monitorInfo = {};
		monitorInfo.cbSize = sizeof(monitorInfo);
		GetMonitorInfo(hMonitor, &monitorInfo);

		if (monitorInfo.dwFlags & MONITORINFOF_PRIMARY)
		{
			*displayBounds = monitorInfo.rcMonitor;
			return FALSE;
		}
		else
			return TRUE;
	}

	void App::OnLaunched([[maybe_unused]] LaunchActivatedEventArgs const& e)
	{
		srand(static_cast<unsigned>(time(nullptr)));
		EnumDisplayMonitors(nullptr, nullptr, MonitorEnumProc, reinterpret_cast<LPARAM>(&displayBounds));
		StartGame();
	}
}
