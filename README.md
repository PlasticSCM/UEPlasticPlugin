Unity Version Control (formerly Plastic SCM) plugin for Unreal Engine
---------------------------------------------------------------------

[![release](https://img.shields.io/github/release/PlasticSCM/UEPlasticPlugin.svg)](https://github.com/PlasticSCM/UEPlasticPlugin/releases)

This is the **official [Unity Version Control (formerly Plastic SCM)](https://unity.com/solutions/version-control) plugin for Unreal Engine 5** (UE 5.0 to 5.5) with previous releases also [supporting UE4.27](https://github.com/PlasticSCM/UEPlasticPlugin/releases/tag/1.11.0).

It is now [distributed in the Unreal Engine Marketplace as a Code Plugin, supporting Engine Versions 5.1 to 5.4 on Windows. ![Unity Version Control in the Unreal Engine Marketplace](Screenshots/Marketplace_UnityVersionControl.png)](https://www.unrealengine.com/marketplace/product/unity-version-control-plastic-scm)

As a general rule, the Marketplace and GitHub versions of the plugin will always be the most up-to-date and recent versions.
They're easier for us to patch by applying a hotfix, in case a bug is raised and something broken must be fixed quickly.

However, you can of course still use the "Plastic SCM" in-engine integration in Unreal.

This plugin is not intended to replace the [Desktop Client](https://docs.plasticscm.com/gui/plastic-scm-version-control-gui-guide)
or [command line interface "cm"](https://docs.unity.com/ugs/en-us/manual/devops/manual/uvcs-cli/version-control-cli).
It is a complementary tool improving efficiency in your daily workflow with assets in Editor.

It tracks status of assets, most notably locks, brings common source control tasks inside the Editor, including updating, branching and merging, and provides visual diffing of Blueprints.
It also helps import an existing Unreal Project into source control in a simple operation, with appropriate *ignore.conf* file.
Since the Unreal Editor does not manage C++ source code, but only assets, the plugin is especially useful for tech designers, level designers and artists in general.

Quick tour "Branching with Unity Version Control (Plastic SCM) in Unreal Engine 5" on YouTube:
[![Branching with Unity Version Control (Plastic SCM) in Unreal Engine 5](https://img.youtube.com/vi/zuyAySbFU98/0.jpg)](https://www.youtube.com/watch?v=zuyAySbFU98)

## Table of Contents

- [User Guide](#user-guide)
  - [Plugin Setup](#plugin-setup)
    - [In-Engine version of the plugin](#in-engine-version-of-the-plugin)
    - [Install from Unreal Engine Marketplace](#install-from-unreal-engine-marketplace)
    - [Install from releases in Github](#install-from-releases-in-github)
    - [Build from sources](#build-from-sources)
  - [Project Setup](#project-setup)
    - [Enable Source Control](#enable-source-control)
    - [Create a new workspace](#create-a-new-workspace--repository-directly-from-unreal)
    - [Source Control settings](#source-control-settings)
    - [Project Settings](#project-settings)
    - [Editor Preferences](#editor-preferences)
  - [Working in Editor](#working-in-editor)
    - [Unreal Documentation](#unreal-documentation)
    - [Status Icons](#status-icons)
    - [Revision Control Menu](#revision-control-menu)
    - [Revision Control Windows](#revision-control-windows)
    - [Redirectors](#redirectors)
    - [Detect Changes on other Branches](#detect-changes-on-other-branches)
    - [Branches](#branches)
    - [Changesets History](#changesets-history)
    - [SmartLocks](#smartLocks)
    - [Merge conflicts on Blueprints](#merge-conflicts-on-blueprints)
    - [Workflows](#workflows)
      - [Mainline](#mainline)
      - [Task branches](#task-branches)
  - [Unity Version Control Setup](#unity-version-control-setup)
    - [Configure Locks for Unreal Assets (exclusive checkout)](#configure-locks-for-unreal-assets-exclusive-checkout)
    - [Configure Visual Diff of Blueprints from Unity Version Control GUI](#configure-visual-diff-of-blueprints-from-unity-version-control-gui)
- [Status](#status)
  - [Feature Requests](#feature-requests)
  - [Known issues](#known-issues)
  - [Features not supported](#features-not-supported)
- [Support](#support)
  - [Enable debug logs](#enable-debug-logs)
    - [Enable Verbose logs in Unreal Engine](#enable-verbose-logs-in-unreal-engine)
    - [Enable logging for Unity Version Control CLI](#enable-logging-for-unity-version-control-cli)
  - [Report an issue](#report-an-issue)
- [Source code architecture](#source-code-architecture)
- [Copyright](#copyright)

## User Guide

### Plugin Setup

#### In-Engine version of the plugin

Having a version of "Plastic SCM" integrated in-Engine helps with discoverability and it is the easiest way to get started with Unity Version Control.
However the integrated version will always be lagging behind the latest release in Github and the Marketplace.

- UE4.24 to 4.27 shipped with an old version 1.4.6 of this plugin
- UE5.0 shipped with the same outdated version 1.4.6, not performing well overall with UE5, especially on OFPA
- UE5.1 shipped with the version 1.6.2 with support for the new View Changelists window
- UE5.2 shipped with the version 1.8.0 with support for the Shelves in the renamed View Changes window
- UE5.3 shipped with the same version 1.8.0
- UE5.4 shipped with the version 1.9.0 with a new View Branches window to create, switch to and merge branches.

#### Install from Unreal Engine Marketplace

As a general rule, the Marketplace and GitHub versions of the plugin will always be the most up-to-date and recent versions.
They're easier for us to patch by applying a hotfix, in case a bug is raised and something broken must be fixed quickly.

[In the Unreal Engine Marketplace, the Unity Version Control plugin supports Engine Versions 5.1 to 5.3](https://www.unrealengine.com/marketplace/product/unity-version-control-plastic-scm)

 1. Click on the "Free" or "Add to cart" button in the Marketplace page and complete the checkout process.*  
[!["Free" and "Add to cart" buttons](Screenshots/Marketplace_FreeAddToCart.png)](https://www.unrealengine.com/marketplace/product/unity-version-control-plastic-scm)
 2. Click the "Open in Launcher" button  
[!["Open in Launcher" button](Screenshots/Marketplace_OpenInLauncher.png)](https://www.unrealengine.com/marketplace/product/unity-version-control-plastic-scm)
 3. Click the "Install to Engine" button, select the Unreal Editor version for your project, then download and install the files.  
[!["Install to Engine" button](Screenshots/Launcher_InstallToEngine.png)](https://www.unrealengine.com/marketplace/product/unity-version-control-plastic-scm)
 4. In your project, navigate to "Edit -> Plugins" and in "Installed" plugins, tick to enable "Unity Version Control", and restart the Unreal Editor  
![Enable the plugin in the Plugins Manager](Screenshots/PluginsManager_InstalledPlugins.png)
 5. Navigate to "Revision Control -> Connect to Revision Control" and for the "Provider" select "UnityVersionControl"  
![Source Control Connect](Screenshots/UEPlasticPlugin-SourceControlDisabled.png)

#### Install from releases in Github

[![release](https://img.shields.io/github/release/PlasticSCM/UEPlasticPlugin.svg)](https://github.com/PlasticSCM/UEPlasticPlugin/releases)

 1. Download the [latest binary release UE5PlasticPlugin-x.x.x.zip](https://github.com/PlasticSCM/UEPlasticPlugin/releases) targeting your UE5 version.
 2. Either:
     1. Unzip the content of the ZIP directly at the root of your project folder.
        This creates a "Plugins\UEPlasticPlugin\" subdirectory into your project.
        This is the way to go to use Unity Version Control on a specific project,
        and to share the plugin with other team members by adding it to source control.
        Some users reported they also had to remove the integrated plugin from "Engine\Plugins\Developer\UnityVersionControl" to avoid a collision.
        This is only needed for some specific use case I have not yet identified (eg. on CI/CD, or on Unix OSes).
     2. Unzip the content of the ZIP in the Engine\ directory of UEX.Y directly for all your projects
        (for instance "C:\Program Files\Epic Games\5.1\Engine\")
        This creates a "UEPlasticPlugin" folder into the "Plugins\" subdirectory.
        Then remove the integrated plugin from "Engine\Plugins\Developer\UnityVersionControl" to avoid the collision.
        This is the way to enable Unity Version Control for all Unreal Engine projects on the machine.
 3. Then, launch your Unreal project, click on the Source Control icon "Connect to Source", select "Plastic SCM".

#### Build from sources

Building from sources enable you to test features before they are released, debug issues, and contribute. It's required if you have your own fork of Unreal Engine.

If your project is already a C++ project, you only have to re-generate Visual Studio project files (step 4 below) and the plugin will get rebuilt the next time you compile your project.

Else, if you want to rebuild the plugin for a Blueprint project:

 0. You need Visual Studio 2015 or 2017 with C++ language support (free Community Edition is fine).
 1. Launch the Unreal Engine Editor, create a new C++ **Basic Code** Project (No Starter Content), for instance ProjectName. This should launch Visual Studio, build the game project, and open it into the Editor.
 2. Close the Editor, then using the file explorer, create a new **Plugins** directory at the root of your project.
 3. Clone the source code of the plugin into this *Plugins* directory (for instance *Plugins\UEPlasticPlugin*).
 4. Right-click on your project's **.uproject** file, **Generate Visual Studio project files**.
 5. In Visual Studio, **Reload All** and **Build Solution** in **Development Editor** mode. That's it, the plugin is built (resulting dlls are located in *Plugins\UEPlasticPlugin\Binaries\Win64*).

To release and redistribute the plugin, zip the *Plugins* folder. But before that, remove the *Intermediate*, *Screenshots* and *.git* folders, and optionnaly the heavier *.pdb files in *Plugins\UEPlasticPlugin\Binaries\Win64*.

### Project Setup

Start by [saving your connection credentials with the Unity Version Control GUI](#save-connection-credentials)

#### Enable Source Control

Launch you Unreal project, look at the Source Control menu at the bottom-right  
![Source Control Connect](Screenshots/UEPlasticPlugin-SourceControlDisabled.png)

Launch you Unreal project, click on the Source Control icon "Connect to Source"  
![Source Control Connect](Screenshots/UEPlasticPlugin-ConnectToSourceControl.png)

Then select "Plastic SCM" plugin  
![Source Control Connect - Select Provider](Screenshots/UEPlasticPlugin-SelectProvider.png)

#### Create a new workspace & repository directly from Unreal

Source Control Login window, to create a new workspace/a new repository, click on "Initialize workspace" (example of a cloud repository):  
![Source Control Login window - create a new workspace on cloud](Screenshots/UEPlasticPlugin-CreateWorkspaceCloud.png)

Or on a server running on premise, using ip:port:  
![Source Control Login window - create a new workspace on localhost](Screenshots/UEPlasticPlugin-CreateWorkspaceOnPremise.png)

This creates an appropriate ignore.conf file, add all relevant files to source control (.uproject, Config & Content subdirectories)
and can also do the initial commit automatically at the end.

Wait for this to succeed before accepting source control settings to not lock the UI & mess with the initialization!  
![Source Control Login window - checking files in source control](Screenshots/UEPlasticPlugin-CheckinInProgress.png)

#### Source Control settings

![Source Control Settings](Screenshots/UEPlasticPlugin-SourceControlSettings.png)

Source control settings can be changed using the Source Control menu,
and are saved locally in `Saved\Config\WindowsEditor\SourceControlSettings.ini`.

- BinaryPath: Path to the Unity Version Control Command Line tool 'cm' binary. Default is good if the cli is in the PATH. Can be changed to an absolute path to the cm executable if needed.
- UpdateStatusAtStartup: Triggers an asynchronous "Update Status" operation at Editor startup. Can take quite some time on big projects, with no source control status available in the meantime.
- UpdateStatusOtherBranches: Enable Update status to detect more recent changes on other branches in order to display the "Changed In Other Branch" warnings and icon.
- EnableVerboseLogs: Override LogSourceControl default verbosity level to Verbose (except if already set to VeryVerbose).

##### Add an ignore.conf file

If you have a project in Unity Version Control but without an ignore.conf file at the root of the workspace,
you can use "Source Control" -> "Change Source Control Settings..." -> "Add a ignore.conf file" button to create a standard one:  
![Source Control Login window - Add a ignore.conf file](Screenshots/UEPlasticPlugin-SourceControlSettings-AddIgnoreConfFile.png)

###### ignore.conf

Content of the generated ignore.conf, to use as a starting point but to adjust to the specific needs of the project:

```
Binaries
Build
DerivedDataCache
Intermediate
Saved
Script
enc_temp_folder
.idea
.vscode
.vs
.ignore
*.VC.db
*.opensdf
*.opendb
*.sdf
*.sln
*.suo
*.code-workspace
*.xcodeproj
*.xcworkspace
```

##### Sharing settings

In order to share this with the team, copy and rename this file into `Config\DefaultSourceControlSettings.ini`,
add it to source control and submit it.

The minimal useful setting would be selecting the proper provider,
but it can be useful to set a few settings if different than the defaults:

```ini
[SourceControl.SourceControlSettings]
Provider=Plastic SCM

[UnityVersionControl.UnityVersionControlSettings]
BinaryPath=cm
UpdateStatusAtStartup=False
UpdateStatusOtherBranches=True
EnableVerboseLogs=False
```

#### Project Settings

##### Source Control

Unreal Engine allows you to configure project-related settings.

- **Should Delete New Files on Revert** (true by default)
  - If enabled, when you revert a file that was added to the project, it will be deleted from disk instead of being left untracked (Private).
    This is the expected behavior when shelving files and reverting the local changes.
- **Enable Uncontrolled Changelists** (true by default)

![Project Settings - Source Control](Screenshots/UEPlasticPlugin-ProjectSettingsSourceControl.png)

##### Source Control - Unity Version Control

The plugin allows you to configure project-related settings.

![Project Settings - Source Control - Unity Version Control](Screenshots/UEPlasticPlugin-ProjectSettingsPlasticSCM.png)

There are 3 settings available at the moment:

- **User Name to Display Name**
  - For each entry in this dictionary, the Editor will replace the user name in the key with the display value you specify.
- **Hide Email Domain in Username** (true by default)
  - This setting toggles the visibility of domain names in user names, if the user name is an email.
- **Prompt for Checkout on Change** (true by default)
  - Un-checking this setting will make the Editor consider all files as already checked out. In that case, you won't get
     any notifications when you modify assets, and the "Checkout Assets" dialog won't show when you save those changes.
     This mimics how Git works, i.e. allowing the user to perform changes without worrying about checking out items.
     Note: Changelists don't currently support locally changed assets (ie not checked-out)
- **Limit Number Of Revisions in History** (50 by default)
  - If a non-null value is set, limit the maximum number of revisions requested to Unity Version Control to display in the "History" window.
  - Requires [Unity Version Control 11.0.16.7608](https://plasticscm.com/download/releasenotes/11.0.16.7608) that added support for history --limit

#### Editor Preferences

##### Source Control

Unreal Engine allows you to configure Editor behaviors. In particular:

- **Tools for diffing text**
  - Configure the external diff tool needed to compare revisions of assets that are not Blueprints (eg Materials)
- **Prompt for Checkout on Asset Modification**
  - Trigger a notification "N files need Checkout" with a link to checkout the asset(s) as soon as a change is made to an asset (without even saving it).
- **Automatically Checkout on Asset Modification**
  - Checkout an asset as soon as a change is made to it (without even saving it). Will not show the "checkout" notification.
- **Add New Files when Modified**
  - Automatically add "private" assets to source control when saving them.
- **Use Global Settings**
  - Share the Source Control Settings ini files between all projects.

![Editor Preferences - Source Control](Screenshots/UEPlasticPlugin-EditorPreferencesSourceControl.png)

### Working in Editor

#### Unreal Documentation

Official documentation from Epic Games:

- [Source Control Inside Unreal Editor](https://docs.unrealengine.com/5.0/en-US/using-source-control-in-the-unreal-editor/)
- [Diffing Unreal Assets (blog post)](https://www.unrealengine.com/en-US/blog/diffing-unreal-assets)

Plastic SCM forums:

- [Unreal Engine section in Plastic SCM forums](https://forum.plasticscm.com/forum/42-unreal-engine/)

#### Status Icons

![New Unsaved](Screenshots/Icons/UEPlasticPlugin-NewUnsaved.png)
![Private/Not in source controlled](Screenshots/Icons/UEPlasticPlugin-Private.png)
![Added to Source Control](Screenshots/Icons/UEPlasticPlugin-Added.png)
![Controlled/Unchanged](Screenshots/Icons/UEPlasticPlugin-Controlled.png)

 1. **New**, unsaved asset (not yet present on disk).
 2. **Private**, the asset is not in source control.
 3. **Added** to source control.
 4. Source **Controlled** but not checked-out nor locally changed

![Changed Locally but not Checked-Out](Screenshots/Icons/UEPlasticPlugin-Changed.png)
![Checked-Out](Screenshots/Icons/UEPlasticPlugin-CheckedOut.png)
![Redirector added by a Move](Screenshots/Icons/UEPlasticPlugin-Redirector.png)
![Moved/Renamed](Screenshots/Icons/UEPlasticPlugin-Renamed.png)

 5. Locally **Changed** without checkout, or **Private** ie not source controlled
 6. **Checked-out** exclusively to prevent others from making modifications (if Locks are enabled on the server)
 7. **Redirector** added by a Move
 8. **Moved** or Renamed

![Checked-Out/Locked by someone else](Screenshots/Icons/UEPlasticPlugin-CheckedOutOther.png)
![Not up-to-date/new revision in repository](Screenshots/Icons/UEPlasticPlugin-NotAtHead.png)
![Newer change in another branch](Screenshots/Icons/UEPlasticPlugin-ChangedInOtherBranch.png)
![Merge Conflict](Screenshots/Icons/UEPlasticPlugin-Conflicted.png)

 9. **Locked somewhere else**, by someone else or in another workspace (if Locks are enabled on the server)
 10. **Not at head revision**, the asset has been submitted with a newer revision on the same branch
 11. **Changed in another branch**, the asset has been changed in a newer changeset in another branch
 12. **Merge conflict**, the asset has been changed in two separate branches and is pending merge resolution

#### Revision Control Menu

##### Unreal Engine 5 Revision Control Menu

(UE5.1) Source Control Menu and status tooltip, extended with commands specific to Unity Version Control:  
![Source Control Menu](Screenshots/UE5PlasticPlugin-SourceControlMenu.png)

(UE5.2+) Revision Control Menu and status tooltip, extended with commands specific to Unity Version Control:  
![Revision Control Menu](Screenshots/UE5PlasticPlugin-RevisionControlMenu.png)

Each Asset Editor also provide some revision control operation, typically to Submit Content:  
![Asset Tools Menu](Screenshots/UEPlasticPlugin-AssetToolsMenu-Diff.png)

The Blueprint Editor also provide a toolbar drop-down to visual diff against a previous revision:  
![Blueprint Toolbar Diff drop-down](Screenshots/UEPlasticPlugin-BlueprintToolbar-Diff.png)

#### Revision Control Windows

Using the Content Browser context revision control sub-menu, you can call in specific actions and windows:  
![Content Browser Context Menu](Screenshots/UEPlasticPlugin-ContentBrowser-ContextMenu-Diff.png)

##### Submit Files

Submit Files to Revision Control window, to check-in selected assets directly (see also Changelists below):  
![Submit Files to Revision Control](Screenshots/UEPlasticPlugin-SubmitFiles.png)

##### View Changelists

Changelists are the new way to group checked-out files by topic in Unreal Engine 5.0, to submit them in coherent batches.
UE5.0 includes Validation checks to ensure there are no missing dependencies outside a changelist, and that all assets are saved on disk before submitting.  
![Changelist Window](Screenshots/UEPlasticPlugin-Changelists.png)

##### File History

File History window, to see the change-log of an asset:  
![History of a file](Screenshots/UEPlasticPlugin-History.png)

##### Blueprint Diff

Visual Diffing of different revision of a Blueprint:  
![Blueprint Visual Diff](Screenshots/UEPlasticPlugin-BlueprintVisualDiff.png)

##### Material Diff

Textual Diffing of a Material:  
![Material Diff](Screenshots/UEPlasticPlugin-MaterialDiff.png)

#### Redirectors

When Source Control is enabled Unreal creates a redirector whenever an asset is renamed or moved,
so that other developers relying on its old name / location can still work with other assets referencing it.

This means, you end up with two files that you have to submit, even if by default they don't show up in the Content Browser.

You can show them in the Content Browser using a dedicated filter:  
![Source Control - Show Redirectors](Screenshots/UE4PlasticPlugin-ShowRedirectors.png)

You can also delete them recursively using the context menu "Fix Up Redirectors in Folder":  
![Source Control - Show Redirectors](Screenshots/UE4PlasticPlugin-FixUpRedirectorsInFolder.png)

#### Detect Changes on other Branches

If you are making use of multiple branches, either for releases and patches, or for tasks or features,
you can enable an option to check for changes in all other branches.

Enable "Update Status" to also checks the history to detect changes on other branches:  
![Source Control Settings](Screenshots/UEPlasticPlugin-SourceControlSettings.png)

Tooltip in the Content Browser when an asset is already checked-out and Locked somewhere else:  
![Asset checked-out by someone else](Screenshots/UEPlasticPlugin-CheckedOutOther-Tooltip.png)

Tooltip in the Content Browser when an asset has been modified in another branch:  
![Asset modified in another branch](Screenshots/UEPlasticPlugin-BranchModification-Tooltip.png)

Tooltip in the Content Browser when an asset has a Retained Smart Lock in another branch:  
![Retained Lock in another branch](Screenshots/UEPlasticPlugin-SmartLocksRetained-Tooltip.png)

Warning when trying to checkout an asset that has been modified in another branch:  
![Warning on checkout for an asset modified in another branch](Screenshots/UEPlasticPlugin-BranchModification-WarningOnCheckout.png)

Warning when trying to modify an asset that has been modified in another branch:  
![Warning on modification for an asset modified in another branch](Screenshots/UEPlasticPlugin-BranchModification-WarningOnModification.png)

#### Branches

The plugin now offers full support for branches, with a new window to list and filter them, and the ability to create, switch to and merge branches from within the Unreal Editor,
reloading assets and the current level as appropriate.

To open it, use the "View Branches" menu item in the Revision Control menu, or click on the name of the current branch in the status bar.

View Branches window:  
![View Branches window](Screenshots/UEPlasticPlugin-Branches-Menu.png)

See the workflows sections below for a discussion about [task branches](#task-branches).

Creating a new child branch:  
![Create Branch Dialog](Screenshots/UEPlasticPlugin-CreateBranch-Dialog.png)

Renaming an existing branch:  
![Rename Branch Dialog](Screenshots/UEPlasticPlugin-RenameBranch-Dialog.png)

Merging a branch into the current one:  
![Merge Branch Dialog](Screenshots/UEPlasticPlugin-MergeBranch-Dialog.png)

Deleting the selected branches:  
![Delete Branches Dialog](Screenshots/UEPlasticPlugin-DeleteBranches-Dialog.png)

##### Changesets History

The plugin has a new dedicated window to display the history of Changesets, to search and filter them, diff them and view their changes, or switch to any of them or their underlying branch.

To open it, use the "View Changesets" menu item in the Revision Control menu.

View history of Changesets window, with a context menu to interact with these changesets:
![View Changesets window](Screenshots/UEPlasticPlugin-Changesets-Menu.png)

It displays the list of files changed in the selected changeset, with contextual operations:
![Files in Changeset context menu](Screenshots/UEPlasticPlugin-ChangesetsFiles-Menu.png)

#### SmartLocks

[Meet Smart Locks, a new way to reduce merge conflicts with Unity Version Control](https://blog.unity.com/engine-platform/unity-version-control-smart-locks)

The plugin now offers full support for Smart Locks, with a new window to list and filter them, and the ability to release or remove them selectively.

To open it, use the "View Locks" menu item in the Revision Control menu.

View Locks window:  
![View Locks window](Screenshots/UEPlasticPlugin-Locks-Menu.png)

Smart Locks administrator context menu to configure lock rules or unlock an asset:  
![Smart Locks admin context menu](Screenshots/UEPlasticPlugin-SmartLocks-Menu.png)

#### Merge conflicts on Blueprints

In case you ever use branches with binary assets without relying on exclusive checkouts (file locks) ([see Workflows below](#workflows))
you will encounter cases of merge conflicts on binary assets.
You have to trigger the resolve in the Unity Version Control GUI, but then skip it without saving changes in order to let the Editor present you with a visual diff.

Branch explorer showing the merge pending with an asset in conflict:  
![Merged branch with a pending conflict resolution](Screenshots/UE4PlasticPlugin-MergeBranch-Pending.png)

Corresponding icon in the Content Browser (only showing after the resolved has been triggered in Unity Version Control):  
![Merge Conflict](Screenshots/Icons/UEPlasticPlugin-NotAtHead.png)

Right click on the asset in conflict to open the *Merge* Tool (just a conflict solver with 3-way Visual Diff, no merge):  
![Merge context menu](Screenshots/UEPlasticPlugin-ContextMenu-Merge.png)

Visual diff of Blueprint properties in conflict:  
![Merge of Blueprint properties](Screenshots/UE4PlasticPlugin-MergeTool-Properties.png)

Visual diff of a Blueprint Event Graph in conflict:  
![Merge of Blueprint Event Graph](Screenshots/UE4PlasticPlugin-MergeTool-EventGraph.png)

#### Workflows

##### Mainline

The most common workflow with Unreal Engine binary assets is the one taught by Perforce:
It relies mostly on one main branch (stream) for everyone with [exclusive checkouts (locks) for the whole Content/ subdirectory](#configure-locks-for-unreal-assets-exclusive-checkout),
in order to prevent merge conflicts on a uasset or a umap file.

This typical workflow would work the best with **the workspace in partial mode** configured using Gluon GUI.
The reason is that a partial workspace enables you to checkin assets without the need to update the workspace before.

1. update the workspace (get latest) using Gluon GUI, with the Unreal Editor closed (since the Editor is locking assets, but also .exe & .dll files that might be in source control)
2. start the Editor, make modifications and checkout assets
3. then check-in (submit) the assets, either from within the Editor, or from the GUI after closing the Editor (the benefit of closing is you ensure that everything is saved)
4. when needed, close the Editor and update the workspace again

If you try to use a full workspace (with Unity Version Control GUI instead of Gluon) you will often need to update the workspace before being able to checkin.

##### Task branches

Handling of binary assets typically works best when working all together in a single main branch (regardless of the source control used).
This is because binary files cannot be merged, and since they increase the cost (time/bandwidth) of switching between branches.

But with Unity Version Control you can use branches that are easy and cheap to create and merge.
Using them for code will enable you to leverage the built-in code review on these branches.
And in combination with [SmartLocks](#smartlocks) and the full in-Editor [Branches](#branches) support,
you can now use them safely for binary assets as well!

Note that some studios have always been using task branches for assets, and included them in their code reviews.
Unity Version Control locks extend to all branches, preventing two people from working at the same time on the same assets regardless of the branch they are one.
The plugin also offers full branch support [and warn users if an asset has been changed in another branch](#branches-support).

To use branches, you would need to also close the Editor before switching from a branch to another, and before merging a branch into another:

1. create a child branch from main using in-Editor Branches window
2. switch to it immediately, updating the workspace, and reloading assets
3. make modifications and checkout assets
4. then check-in the assets (remember to save everything, the toolbar now has a button to track the number of unsaved assets)
6. create a code review from the branch
7. create a new task branch from main or go back to main to merge branches

The plugin also leverages the [visual diff merge conflicts resolution tool from the Editor](#merge-conflicts-on-blueprints) but this is a pain and isn't working as expected currently (as of 1.5.0 & UE5.0)

### Unity Version Control Setup

#### Save connection credentials

The plugin works with the command line interface "cm" that currently requires your credentials to be stored.
Use the Desktop Client or Gluon GUI to enter and save your credentials before enabling Unity Version Control in Unreal.

#### Configure Locks for Unreal Assets (exclusive checkout)

[Administrator guide - Chapter 7: Configuring exclusive checkout (Lock)](https://docs.plasticscm.com/administration/plastic-scm-version-control-administrator-guide#Chapter7:Configuringexclusivecheckout(Lock))

Binary assets should be locked for exclusive access to avoid merge conflicts.

To lock all assets on the whole `Content` directory, you need to put a `lock.conf` in your server directory (for instance `C:\Program Files\PlasticSCM5\server`) with this content:

    rep:<repname> [br:[<destination_branch>]] [excluded_branches:<exclusion_pattern>…]
    /Content

For instance a specific ruleset to one repository:

    rep:UE5PlasticPluginDev
    /Content

or with multiple destination branches for Locks:

    rep:UE5PlasticPluginDev br:/main /main/release excluded_branches:/main/experiments
    /Content

or using file extensions instead of a path:

    rep:UE5PlasticPluginDev
    *.uasset
    *.umap

The more generic config applying to all respository on the server would be:

    rep: *
    /Content

But beware, as this default global rule will not be used (merged) with any other rules specific repository, but completely replaced ("overridden") by them.

On Unity Version Control Cloud, you can just set-up lock rules like that:

    /Content

#### Configure Visual Diff of Blueprints from Unity Version Control GUI

In "Preferences -> Diff tools" add a new config for uasset and move it up **before** the existing `$binary` one:  
![Diff tools](Screenshots/UEPlasticPlugin-GUIDiffToolsUasset.png)

The command line needs the quoted path to the UnrealEditor.exe, the quoted patch to your ".uproject" file, -diff, then the source & destination files variables also quoted

    "C:\Program Files\Epic Games\UE_5.0\Engine\Binaries\Win64\UnrealEditor.exe" "C:\wkspaces\ProjectName\ProjectName.uproject" -diff "@sourcefile" "@destinationfile"

eg:

    "C:\Program Files\Epic Games\UE_5.2\Engine\Binaries\Win64\UnrealEditor.exe" "C:\UnitySrc\UE5UnityVCSDevEnv\UE5UnityVCSDevEnv.uproject" -diff "@sourcefile" "@destinationfile"

#### Text Diff of any assets

To configure a text diff for any uasset (not only Blueprints) use this command instead

    "C:\Program Files\Epic Games\UE_5.0\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "C:\wkspaces\ProjectName\ProjectName.uproject" -NoShaderCompile -run="DiffAssets" %1 %2 DiffCmd="C:\Program Files\PlasticSCM5\client\mergetool.exe -s={1} -d={2}"

eg:

    DiffCmd="C:\Program Files\Perforce\p4merge.exe {1} {2}"

    "C:\Program Files\Epic Games\UE_5.0\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "C:\UnitySrc\UE5UnityVCSDevEnv\UE5UnityVCSDevEnv.uproject" -NoShaderCompile -run="DiffAssets" %1 %2 DiffCmd="C:\Program Files\PlasticSCM5\client\mergetool.exe -s={1} -d={2}"

## Status

This version here is the development version, so it always contains additional fixes, performance improvements or new features compared to the one integrated in Engine.

### Version 1.11.0 2024/06/18 for UE 5.0/5.1/5.2/5.3/5.4 and UE 4.27

[![releases](https://img.shields.io/github/release/PlasticSCM/UEPlasticPlugin.svg)](https://github.com/PlasticSCM/UEPlasticPlugin/releases)
[![1.11.0](https://img.shields.io/github/v/tag/PlasticSCM/UEPlasticPlugin?filter=1.11.0)](https://github.com/PlasticSCM/UEPlasticPlugin/releases/tag/1.11.0)

 - manage connection to the server
 - display status icons to show controlled/checked-out/added/deleted/private/changed/ignored/not-up-to-date files
 - Smart Locks: manage Locks from a dockable window. Release or Remove them. Display status icons for locked files, retained locks, on what branch and by whom.
 - Branches: manage branches from a dockable window. Create, rename, switch, merge and delete.
 - Detect Changes on other Branches, to check outdated files vs. remote across multiple branches
 - show current branch name and CL in status text
 - add, duplicate a file
 - move/rename a file or a folder
 - revert modifications of a file (works best with the "Content Hot-Reload" option since UE4.15)
 - check-in a set of files with a multi-line UTF-8 comment
 - migrate (copy) an asset between two projects if both are using Unity Version Control
 - delete file (but no way to check-in them, see known issues below)
 - update workspace to latest head (Sync command)
 - show history of a file
 - visual diff of a blueprint against depot or between previous versions of a file
 - Changelists and Shelves in Unreal Engine 5: create, edit, move files, shelves and unshelve, revert, delete
 - One Files Per Actor (OFPA) in Unreal Engine 5: status batching to execute only one operation for all files in all subfolders
 - initialize a new workspace to manage your Unreal Engine Game Project.
   - make the initial commit with a custom message
   - create an appropriate ignore.conf file, either as part of initialization or later
 - show merge conflict files with 3-way visual diff (but not resolving conflicts yet)
 - "Source Control" menu global actions
   - "Sync/update workspace" instead of on folder's context menu
   - "Revert Unchanged" and "Revert All"
   - "Refresh" to update local source control status icons (but doesn't re-evaluate locks or changes made on other branches)
 - [Partial Workspace (Gluon, for artists)](https://docs.plasticscm.com/gluon/plastic-scm-version-control-gluon-guide) [see also](http://blog.plasticscm.com/2015/03/plastic-gluon-is-out-version-control.html)
 - Plastic Cloud is fully supported
 - xlinks sub-repositories (for Plugins for instance)
 - Toggle verbose logs from the Source Control settings UI
 - Run 'cm' CLI commands directly from the Unreal Editor Console, Blueprints of C++ code.
 - Supported on Windows and Linux

### Feature Requests
 - Mac OS X Support
 - add a setting to configure Unity Version Control to use "read-only flags" like Perforce
   - add a context menu entry to make it locally writable
 - add a menu entry to unlock a file
 - add a setting to pass the --update option to "check-in"
 - add a "clean directory"
 - Solve a merge conflict on a blueprint (see Known issues below)
 - Shelves on Xlinks sub-repositories

### Known issues

- Merge Conflict: "Accept Target" crash the UE4 & UE5 Editor (same with Perforce and Git Plugins)
- Merge conflict from cherry-pick or from range-merge cannot be solved by the plugin: use the Unity Version Control GUI
- Bug: the Editor does not handle visual diff for renamed/moved assets

### Features not supported

Some are reserved for internal use by Epic Games with Perforce only:

- tags: get labels (used for crash when the full Engine is under Perforce)
- annotate: blame (used for crash when the full Engine is under Perforce)

## Support

You can always ask for support in:
 - [Unity Version Control support](https://support.unity.com/hc/en-us/requests/new?ticket_form_id=360001051792)
 - [Unity Version Control forums](https://forum.unity.com/forums/unity-version-control.605/)
 - [Unreal Engine forums](https://forums.unrealengine.com/)
 - [Unreal Slackers Discord community](https://discord.gg/unreal-slackers)

### Enable debug logs

#### Enable Verbose logs in Unreal Engine

To help diagnose any issue related to the plugin or to the Editor, toggle Verbose logs for *LogSourceControl* in [Source Control settings](#source-control-settings).

Unreal log files are in `<ProjectName>/Save/Logs/ProjectName.log`.

#### Enable logging for Unity Version Control CLI

To help diagnose any issue related to the underlying Unity Version Control "cm" Command Line Interface,
enable debug logs for the CLI client cm.exe:

Copy [the configuration file cm.log.conf](https://raw.githubusercontent.com/PlasticSCM/UEPlasticPlugin/master/cm.log.conf) in the directory where the cm.exe resides, by default in `C:\Program Files\PlasticSCM5\client\`.

cm log files are then stored in `<LOCALAPPDATA>\plastic4\logs\cm.log.txt`.

(from [enable logging for Plastic SCM](https://docs.plasticscm.com/technical-articles/kb-enabling-logging-for-plastic-scm-part-i))

### Report an issue

To report an issue, please create a support ticket as it helps sort the priorities [Unity Version Control support](https://support.unity.com/hc/en-us/requests/new?ticket_form_id=360001051792).

You can also use the [Github issue-tracker](https://github.com/SRombauts/UEPlasticPlugin/issues?q=is%3Aissue).

 1. Have a look at existing issues (Open and Closed ones)
 2. Specify your Engine & Plugin versions, and if either are built from sources
 3. Describe precisely your issue
 4. Add reproduction steps, if possible on a basic template project
 5. Post log files whenever possible (see [enable debug logs](#enable-debug-logs) to get the most of it)
    1. Unreal Log file **`<ProjectName>/Saved/Logs/ProjectName.log`**
    2. cm debug log file typically from **`<LOCALAPPDATA>\plastic4\logs\cm.log.txt`**

### Use merge requests

If you want to help, [Github Pull Requests](https://github.com/PlasticSCM/UEPlasticPlugin/pulls) are welcome!

## Source code architecture

See also [Unreal Engine C++ Coding Standard](https://dev.epicgames.com/documentation/en-us/unreal-engine/epic-cplusplus-coding-standard-for-unreal-engine)

All the relevant C++ source code of the plugin reside in one subdirectory `<ProjectName>/Plugins/UEPlasticPlugin/Source/UnityVersionControl/Private/`

### Implementations of all the Source Control APIs as C++ interfaces

 - **UnityVersionControlProvider**.cpp/.h
   - `class FUnityVersionControlProvider : public ISourceControlProvider`
   - implements the high level source control interface with the mechanism around managing workspace states
 - **UnityVersionControlOperations**.cpp/.h
   - `classes FPlastic<Operation> : public ISourceControlOperation`
   - implements each source control operation with a dedicated Worker class: add, delete, move, checkout, checkin, revert etc, see eg:
   - `bool FPlasticCheckOutWorker::Execute(FUnityVersionControlCommand& InCommand)` using the following two classes:
     - **IUnityVersionControlWorker**.h
       - `class IUnityVersionControlWorker`
       - interface of one Worker to be implemented for each of the operations
     - **UnityVersionControlCommand**.cpp/.h
       - `class FUnityVersionControlCommand : public IQueuedWork`
       - describes the parameters of the work to be executed for one operation
 - **UnityVersionControlState**.cpp/.h
   - `class FUnityVersionControlState : public ISourceControlState`
   - implements information about the state of a file
 - **UnityVersionControlRevision**.cpp/.h
   - `class FUnityVersionControlRevision : public ISourceControlRevision`
   - implements information about a revision in the history of a file
 - **UnityVersionControlChangelist**.cpp/.h
   - `class FUnityVersionControlChangelist : public ISourceControlChangelist`
   - Unique Identifier of a changelist under source control: a "name" in Unity Version Control
 - **UnityVersionControlChangelistState**.cpp/.h
   - `class FUnityVersionControlChangelistState : public ISourceControlChangelistState`
   - The state of a pending changelist under source control: description and list of files

### Other most relevant structural files

 - **UnityVersionControlModule**.cpp/.h
   - `class FUnityVersionControlModule : public IModuleInterface`
   - Singleton-like entry point of the plugin
 - **UnityVersionControlUtils**.cpp/.h
   - `namespace UnityVersionControlUtils` with free functions
   - functions wrapping "cm" operations, and their the dedicated parsers (eg "status", "history" etc.)
 - **UnityVersionControlShell**.cpp/.h
   - `namespace UnityVersionControlShell` with free functions and internal static variables
   - low level wrapper around the "cm shell" background process
 - **SUnityVersionControlSettings**.cpp/.h
   - `class SUnityVersionControlSettings : public SCompoundWidget`
   - the "Source Control Login" window shown above: to enable the plugin, and with a wizard to create the workspace
 - **UnityVersionControlSettings**.cpp/.h
   - serialize the 4 settings displayed in the Source Control Login in `Saved\Config\WindowsEditor\SourceControlSettings.ini`

### All the others providing various features

 - **UnityVersionControlMenu**.cpp/.h
   - extends the main source control menu, now in the status bar at the bottom of the Editor
 - **UnityVersionControlProjectSettings**.h
   - add a section "Editor - Source Control - Unity Version Control" to the Project Settings (saved in `Config\DefaultEditor.ini`)
 - **UnityVersionControlConsole**.cpp/.h
   - add a console command that can be executed from the Editor status bar or Output Log to execute "cm" commands in order to query Unity Version Control, eg:
   - `cm location`
   - `cm find revision "where item='Content/ThirdPerson/Blueprints/BP_ThirdPersonCharacter.uasset'"`
 - **ScopedTempFile**.cpp/.h
   - Helper for temporary files to pass as arguments to some commands (typically for checkin multi-line text message)
 - **SoftwareVersion**.cpp/.h
   - Software version string in the form "X.Y.Z.C", ie Major.Minor.Patch.Changeset (as returned by GetPlasticScmVersion)
 - **UnityVersionControlUtilsTests**.cpp

## Copyright

Copyright (c) 2023 Unity Technologies
