#pragma once

///
/// This should act as a clicker without clicking 
///
class VisualizeClicker
{
public:
	VisualizeClicker();
	~VisualizeClicker();

public:
	float d_time = 0;

	void Start();
	void Stop();

	bool IsStarted() const;

	float GetCPS() const;
	toadll::Randomization GetRand();

	void SetRand(const toadll::Randomization& rand);

	void SetClickCallback(const std::function<void()>& f);

private:
	void clicking_thread();

	// same as clicker
	void click_down();
	void click_up();

	void apply_rand(std::vector<toadll::Inconsistency>& inconsistencies);
	void update_rand();

private:
	toadll::Timer m_rand_delay_timer;

	std::queue<toadll::Timer> m_click_queue;

	std::thread m_clicking_thread;
	std::atomic_bool m_thread_running = false;

	// same as left clicker 
	toadll::Randomization m_rand = toad::left_clicker::rand;

	std::function<void()> m_callback;
};

