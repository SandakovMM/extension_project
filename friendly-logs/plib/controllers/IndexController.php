<?php

class IndexController extends pm_Controller_Action
{
    public function init()
    {
        parent::init();
        $this->view->pageTitle = 'Example Module';

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
	    array(
                'title' => 'Tools',
                'action' => 'tools',
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
	

	$sys_answer = system ('pwd');
        // Init form here
        $form = new pm_Form_Simple();
        $form->addElement('text', 'exampleText', array(
            'label' => $sys_answer,
            'value' => pm_Settings::get('exampleText'),
            'required' => true,
            'validators' => array(array('NotEmpty', true),),
        ))
	     ->addElement('submit', 'geted', array(
		'attribs' => array(
		'onclick' => 'alert("Hello!")'
	)));
		
        $this->view->form = $form;
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
