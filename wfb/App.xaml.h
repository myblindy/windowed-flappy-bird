#pragma once

#include "App.xaml.g.h"

namespace winrt::wfb::implementation
{
	struct App : AppT<App>
	{
		App();

		void OnLaunched(Microsoft::UI::Xaml::LaunchActivatedEventArgs const&);

	private:
		RECT displayBounds;

		void PositionWindowPercentage(winrt::Microsoft::UI::Xaml::Window const& window, double px, double py);
		void SizeWindowPercentage(winrt::Microsoft::UI::Xaml::Window const& window, double pw, double ph);

		void HandleWindowClose(const winrt::Microsoft::UI::Xaml::Window& window);

		winrt::Microsoft::UI::Xaml::DispatcherTimer flapTimer{ nullptr }, logicTimer{ nullptr };
		bool allowWindowClose{};

		void StartGame();
		void GameOver();

		void CreateScoreWindow();
		winrt::Microsoft::UI::Xaml::Window scoreWindow{ nullptr };
		winrt::Microsoft::UI::Xaml::Controls::TextBlock scoreText{ nullptr };

		int score{};
		void UpdateScore() { scoreText.Text(L"Score: " + to_hstring(score)); }

		const double pBirdW = 0.05, pBirdH = 0.1;
		void CreateBirdWindow();
		winrt::Microsoft::UI::Xaml::Window birdWindow{ nullptr };
		std::vector<winrt::Microsoft::UI::Xaml::Controls::Image> birdImages;
		winrt::Microsoft::UI::Xaml::Media::RotateTransform birdRotateTransform;

		double pBirdX = 0.1, pBirdY = 0.5, pBirdVY{}, pBirdAY = 0.001;
		winrt::Windows::Foundation::DateTime lastBirdJumpTime{};
		void UpdateBirdPosition() { PositionWindowPercentage(birdWindow, pBirdX, pBirdY - pBirdH / 2); }

		int birdFrame{};
		void UpdateBirdFrame();

		std::vector<winrt::Microsoft::UI::Xaml::Window> pipeWindows, freePipeWindows;
		std::vector<double> pPipeXs, pPipeYs, pPipeHs;
		const double pPipeW = 0.1, pPipeGapH = 0.3, pPipeVX = -0.005;
		void CreatePipeWindows(double px);
	};
}
