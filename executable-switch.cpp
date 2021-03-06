#include "advanced-scene-switcher.hpp"

int SceneSwitcher::executableFindByData(const QString& exe)
{
	int count = ui->executables->count();

	for (int i = 0; i < count; i++)
	{
		QListWidgetItem* item = ui->executables->item(i);
		QString itemExe = item->data(Qt::UserRole).toString();

		if (itemExe == exe)
			return i;
	}

	return -1;
}


void SceneSwitcher::on_executables_currentRowChanged(int idx)
{
	if (loading)
		return;
	if (idx == -1)
		return;

	QListWidgetItem* item = ui->executables->item(idx);

	QString exec = item->data(Qt::UserRole).toString();

	lock_guard<mutex> lock(switcher->m);
	for (auto& s : switcher->executableSwitches)
	{
		if (exec.compare(s.mExe) == 0)
		{
			QString sceneName = GetWeakSourceName(s.mScene).c_str();
			QString transitionName = GetWeakSourceName(s.mTransition).c_str();
			ui->executableScenes->setCurrentText(sceneName);
			ui->executable->setCurrentText(exec);
			ui->executableTransitions->setCurrentText(transitionName);
			ui->requiresFocusCheckBox->setChecked(s.mInFocus);
			break;
		}
	}
}


void SceneSwitcher::on_executableAdd_clicked()
{
	QString sceneName = ui->executableScenes->currentText();
	QString exeName = ui->executable->currentText();
	QString transitionName = ui->executableTransitions->currentText();
	bool inFocus = ui->requiresFocusCheckBox->isChecked();

	if (exeName.isEmpty() || sceneName.isEmpty())
		return;

	OBSWeakSource source = GetWeakSourceByQString(sceneName);
	OBSWeakSource transition = GetWeakTransitionByQString(transitionName);
	QVariant v = QVariant::fromValue(exeName);

	QString text = MakeSwitchNameExecutable(sceneName, exeName, transitionName, inFocus);

	int idx = executableFindByData(exeName);

	if (idx == -1)
	{
		lock_guard<mutex> lock(switcher->m);
		switcher->executableSwitches.emplace_back(
			source, transition, exeName.toUtf8().constData(), inFocus);

		QListWidgetItem* item = new QListWidgetItem(text, ui->executables);
		item->setData(Qt::UserRole, v);
	}
	else
	{
		QListWidgetItem* item = ui->executables->item(idx);
		item->setText(text);

		{
			lock_guard<mutex> lock(switcher->m);
			for (auto& s : switcher->executableSwitches)
			{
				if (s.mExe == exeName)
				{
					s.mScene = source;
					s.mTransition = transition;
					s.mInFocus = inFocus;
					break;
				}
			}
		}

		ui->executables->sortItems();
	}
}


void SceneSwitcher::on_executableRemove_clicked()
{
	QListWidgetItem* item = ui->executables->currentItem();
	if (!item)
		return;

	QString exe = item->data(Qt::UserRole).toString();

	{
		lock_guard<mutex> lock(switcher->m);
		auto& switches = switcher->executableSwitches;

		for (auto it = switches.begin(); it != switches.end(); ++it)
		{
			auto& s = *it;

			if (s.mExe == exe)
			{
				switches.erase(it);
				break;
			}
		}
	}

	delete item;
}

void SwitcherData::checkExeSwitch(bool& match, OBSWeakSource& scene, OBSWeakSource& transition)
{
	QStringList runningProcesses;
	GetProcessList(runningProcesses);
	for (ExecutableSceneSwitch& s : executableSwitches)
	{
		if (runningProcesses.contains(s.mExe))
		{
			scene = s.mScene;
			transition = s.mTransition;
			match = !s.mInFocus || (s.mInFocus && isInFocus(s.mExe));
			break;
		}
	}
}
