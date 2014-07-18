<?php

class IndexController extends pm_Controller_Action
{
    private $checkGroup = null;
    private $logNamesGroup = null;
    public function init()
    {
        parent::init();
        $this->view->pageTitle = 'Example Module';
	// Include js files.
	$scriptURL = pm_Context::getBaseUrl() . 'js/uiScript.js';
	$this->view->headScript()->appendFile($scriptURL);
        // Init tabs for all actions
        $this->view->tabs = array(
            array(
                'title' => 'Form',
                'action' => 'form',
            ),
	    array(
                'title' => 'List',
                'action' => 'list',
            ),
        );
    }

    public function indexAction()
    {
        // Default action will be formAction
		$this->_forward('form');
    }

    public function formAction()
    {
        // Display simple text in view
        $this->view->test = 'This is index action for testing module.';

        $form = new pm_Form_Simple(); // init form class

        // Add text and button elements
        /*$form->addElement('text', 'exampleText', array(
            'label' => 'Somthing in here',
            'value' => pm_Settings::get('exampleText'),
            'required' => true,
            'validators' => array(array('NotEmpty', true),),
        ))
	     ->addElement('submit', 'geted', array(
		'attribs' => array(
		'onclick' => 'alert("Hello!")'
	)));/**/
	//$form->addDisplayGroup(array('exampleText', 'geted'), 'check', array('legend'=>'Checking'));
	
	$this->castFileNamesGroup($form);
	$this->castLogOutputPlace($form);


        $this->view->form = $form;
    }
   
    private function castFileNamesGroup($form) // add checks with log file name to a form
    {
    	$this->logNamesGroup = $form->getDisplayGroup('group');
    	// Geting domain name what we work.
    	$domain = pm_Session::getCurrentDomain();
    	$domainName = $domain->getName();
    	
    	// get needed names with shell command ls
    	$logs_names = scandir/*glob*/('/var/www/vhosts/system/' .
    		$domainName . '/logs');
    	$logNameArray = array();
    	// Make display group with log file names
    	foreach ($logs_names as $one_name) {
    		if (!empty($one_name) && '.' != $one_name && '..' != $one_name) {
    			$counter = $counter + 1;	// Make element
    			$checkbox = $form->createElement('checkbox', 'log' . $counter,
    					 array('label' => $one_name));
                $checkbox->setCheckedValue($one_name);
    			$checkbox->setAttrib('onclick', 'Reaction.clickFileCheck(this)');
    			$form->addElement($checkbox);
    				// Add element to group
    			if (is_null($this->logNamesGroup)) {
    				$logNameArray[] = $checkbox->getName();
    				$form->addDisplayGroup($logNameArray, 'group', array('legend'=>'Log file names'));
    				$this->logNamesGroup = $form->getDisplayGroup('group');
    			}
    			$this->logNamesGroup->addElement($checkbox);/* Find the way how do it normal*/			
    		}
    	}

    	// Last button, who must set all others seted or somthing like this
    	$checkbox = $form->createElement('checkbox', 'setupAll', array(
    			'label' => 'Total count is:' . $counter));
        $checkbox->setCheckedValue($counter);
    	$checkbox->setAttrib('onclick', 'Reaction.clickAllFiles(this)');
    	$form->addElement($checkbox);
    	$this->logNamesGroup->addElement($checkbox);/* Find the way how do it normal*/

    	// Decorator for our display group with checkboxes
    	$this->logNamesGroup->setDecorators(array(
    		'FormElements', 'Fieldset',
    		array('HtmlTag', array('tag'=>'div', 'style'=>'width:30%;height:80vh;float:left;
    										overflow-y:auto;'))
    	));
    }
    private function castLogOutputPlace($form)
    {
    	$selectElem = new Zend_Form_Element_Select('logList', array(
    			'label'=>'Logs', 'size'=>'2', 
    			'style'=>'width:65%;height:80vh;float:right;overflow-y:auto;'));
    	$selectElem->addMultiOptions(array('0'=>'some','1'=>'other','2'=>'fuck','3'=>'you', '4'=>'ok?'));
    	$form->addElement($selectElem);
    }
    public function listAction()
    {
        $list = $this->_getNumbersList();
        // List object for pm_View_Helper_RenderList
        $this->view->list = $list;
    }

    public function listDataAction()
    {
        $list = $this->_getNumbersList();
        // Json data from pm_View_List_Simple
        $this->_helper->json($list->fetchData());
    }

    private function _getNumbersList()
    {
        $data = array();
        $iconPath = pm_Context::getBaseUrl() . 'images/icon_16.gif';
        for ($index = 1; $index < 150; $index++) {
            $data[] = array(
                'column-1' => '<a href="#">link #' . $index . '</a>',
                'column-2' => '<img src="' . $iconPath . '" /> image #' . $index,   );
        }

        $list = new pm_View_List_Simple($this->view, $this->_request);
        $list->setData($data);
        $list->setColumns(array('column-1' => array(
                'title' => 'Link',
                'noEscape' => true,
                'searchable' => true,  ),
            'column-2' => array(
                'title' => 'Description',
                'noEscape' => true,
                'sortable' => false,   ),
        ));

        // Take into account listDataAction corresponds to the URL /list-data/
        $list->setDataUrl(array('action' => 'list-data'));
        return $list;
    }

    public function toolsAction()
    {
        // Tools for pm_View_Helper_RenderTools
        $this->view->tools = array(
            array('icon' => pm_Context::getBaseUrl()."img/site-aps_32.gif",
                'title' => 'Example',
                'description' => 'Example extension with UI samples',
                'link' => pm_Context::getBaseUrl(),   ),
            array('icon' => pm_Context::getBaseUrl()."img/modules_32.gif",
                'title' => 'Extensions',
                'description' => 'Extensions installed in Plesk',
                'link' => pm_Context::getModulesListUrl(),     ),
        );
    }
}
